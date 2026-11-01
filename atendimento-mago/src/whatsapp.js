// Ligação com o WhatsApp do Mago pela biblioteca Baileys (conexão não oficial,
// o programa entra como um "aparelho conectado", igual ao WhatsApp Web).
import fs from 'node:fs/promises'
import QRCode from 'qrcode'
import {
  makeWASocket, useMultiFileAuthState, DisconnectReason, Browsers, downloadMediaMessage,
  normalizeMessageContent, getContentType, jidNormalizedUser, isJidGroup, generateMessageIDV2,
  fetchLatestBaileysVersion
} from 'baileys'
import { soDigitos } from './util.js'

const IGNORAR_TIPOS = new Set(['protocolMessage', 'reactionMessage', 'senderKeyDistributionMessage',
  'pollUpdateMessage', 'messageContextInfo', 'editedMessage', 'keepInChatMessage', 'pinInChatMessage'])
const DEZ_MINUTOS = 10 * 60

const esperar = (ms) => new Promise(r => setTimeout(r, ms))

function lembrar (conjunto, valor, max = 5000) {
  conjunto.add(valor)
  if (conjunto.size > max) conjunto.delete(conjunto.values().next().value)
}

export function criarWhatsapp ({ pastaSessao, banco, log }) {
  const estado = { situacao: 'iniciando', qr: null, numero: null }
  let sock = null
  let assistente = null
  let reconectando = false
  let falhas = 0
  const enviadasPeloAssistente = new Set()
  const jaVistas = new Set()
  const cacheJid = new Map()

  const eu = () => [sock?.user?.id, sock?.user?.lid].filter(Boolean).map(j => jidNormalizedUser(j))
  const meuJid = () => jidNormalizedUser(sock.user.id)

  async function conectar () {
    reconectando = false
    const { state, saveCreds } = await useMultiFileAuthState(pastaSessao)
    let version
    try { ({ version } = await fetchLatestBaileysVersion()) } catch { /* usa a versão embutida */ }
    sock = makeWASocket({
      auth: state,
      version,
      logger: log.child({ modulo: 'baileys' }, { level: 'error' }),
      browser: Browsers.ubuntu('Chrome'),
      markOnlineOnConnect: false,
      syncFullHistory: false
    })
    sock.ev.on('creds.update', saveCreds)
    sock.ev.on('connection.update', aoMudarConexao)
    sock.ev.on('messages.upsert', async ({ messages, type }) => {
      for (const m of messages) {
        try { await tratar(m, type) } catch (e) { log.error({ err: e }, 'falha ao tratar mensagem') }
      }
    })
  }

  async function aoMudarConexao ({ connection, lastDisconnect, qr }) {
    if (qr) {
      estado.situacao = 'aguardando_pareamento'
      estado.qr = qr
      falhas = 0
      log.info('WhatsApp aguardando conexão. Leia o QR abaixo ou peça o código de pareamento no painel.')
      console.log(await QRCode.toString(qr, { type: 'terminal', small: true }))
    }
    if (connection === 'open') {
      estado.situacao = 'conectado'
      estado.qr = null
      estado.numero = soDigitos(sock.user.id.split(':')[0].split('@')[0])
      falhas = 0
      log.info({ numero: estado.numero }, 'WhatsApp conectado')
    }
    if (connection === 'close' && !reconectando) {
      reconectando = true
      const codigo = lastDisconnect?.error?.output?.statusCode
      if (codigo === DisconnectReason.loggedOut) {
        log.warn('O WhatsApp desconectou este aparelho. Será preciso conectar de novo.')
        estado.situacao = 'desconectado'
        estado.numero = null
        await fs.rm(pastaSessao, { recursive: true, force: true })
      } else {
        estado.situacao = 'reconectando'
      }
      // Espera cada vez mais entre tentativas (3s, 6s, 12s... até 1 minuto) para não insistir demais.
      const espera = Math.min(60000, 3000 * 2 ** falhas++)
      if (estado.situacao === 'reconectando') log.warn({ codigo, espera }, 'conexão caiu, tentando de novo')
      setTimeout(() => conectar().catch(e => log.error({ err: e }, 'falha ao reconectar')), espera)
    }
  }

  async function jidDoTelefone (telefone) {
    const d = soDigitos(telefone)
    if (!d) return null
    if (cacheJid.has(d)) return cacheJid.get(d)
    let jid = null
    try {
      const [r] = await sock.onWhatsApp(d)
      if (r?.exists) jid = jidNormalizedUser(r.jid)
    } catch (e) { log.warn({ err: e }, 'não consegui consultar o número') }
    if (jid) cacheJid.set(d, jid)
    return jid
  }

  // Para onde vão os avisos de pedido: o número de aviso, se houver, ou a conversa do Mago consigo mesmo.
  async function destinoMago () {
    const aviso = banco.ajuste('aviso_numero')
    if (aviso) {
      const jid = await jidDoTelefone(aviso)
      if (jid) return jid
      log.warn('número de aviso não encontrado no WhatsApp; usando a conversa do Mago consigo mesmo')
    }
    return meuJid()
  }

  async function mandar (jid, conteudo) {
    if (!sock || estado.situacao !== 'conectado') throw new Error('WhatsApp desconectado')
    const id = generateMessageIDV2(sock.user?.id)
    lembrar(enviadasPeloAssistente, id)
    await sock.sendMessage(jid, conteudo, { messageId: id })
  }

  async function digitando (jid, texto) {
    if (eu().includes(jid)) return
    try { await sock.sendPresenceUpdate('composing', jid) } catch { /* sem importância */ }
    await esperar(Math.min(2000, 500 + texto.length * 10))
  }

  function conteudoArquivo ({ caminho, mimetype, legenda }) {
    if (/^image\//.test(mimetype)) return { image: { url: caminho }, mimetype, caption: legenda }
    return { document: { url: caminho }, mimetype, fileName: caminho.split('/').pop(), caption: legenda }
  }

  const envio = {
    async enviarTexto (jid, texto) {
      await digitando(jid, texto)
      await mandar(jid, { text: texto })
    },
    enviarArquivo: (jid, arq) => mandar(jid, conteudoArquivo(arq)),
    enviarTextoMago: async (texto) => mandar(await destinoMago(), { text: texto }),
    enviarArquivoMago: async (arq) => mandar(await destinoMago(), conteudoArquivo(arq))
  }

  async function tratar (m, tipoEvento) {
    const k = m.key
    if (!k?.remoteJid || !m.message || !k.id) return
    if (jaVistas.has(k.id) || enviadasPeloAssistente.has(k.id)) return
    lembrar(jaVistas, k.id)

    const remoto = k.remoteJid
    if (isJidGroup(remoto) || remoto.endsWith('@broadcast') || remoto.endsWith('@newsletter')) return
    // Com os novos identificadores do WhatsApp (LID), o número de telefone vem em remoteJidAlt.
    const jid = jidNormalizedUser(remoto.endsWith('@lid') && k.remoteJidAlt ? k.remoteJidAlt : remoto)

    const conteudo = normalizeMessageContent(m.message)
    const tipo = getContentType(conteudo)
    if (!tipo || IGNORAR_TIPOS.has(tipo)) return
    const texto = conteudo.conversation || conteudo.extendedTextMessage?.text || ''
    const idade = Date.now() / 1000 - Number(m.messageTimestamp || 0)

    if (k.fromMe) {
      if (idade > DEZ_MINUTOS) return // mensagens antigas reenviadas depois de reconectar
      if (eu().includes(jid) || eu().includes(jidNormalizedUser(remoto))) {
        if (texto) await assistente.comandoDoMago(texto)
      } else {
        await assistente.magoRespondeuCliente(jid)
      }
      return
    }
    if (tipoEvento !== 'notify') return

    const aviso = banco.ajuste('aviso_numero')
    if (aviso && jid === await jidDoTelefone(aviso)) {
      if (texto) await assistente.comandoDoMago(texto)
      return
    }

    const msg = { jid, nomeWhatsapp: m.pushName, texto, tipo: 'outro' }
    if (tipo === 'conversation' || tipo === 'extendedTextMessage') msg.tipo = 'texto'
    else if (tipo === 'imageMessage') { msg.tipo = 'imagem'; msg.mimetype = conteudo.imageMessage.mimetype }
    else if (tipo === 'documentMessage') { msg.tipo = 'documento'; msg.mimetype = conteudo.documentMessage.mimetype }
    else if (tipo === 'audioMessage') msg.tipo = 'audio'
    if (msg.tipo === 'imagem' || msg.tipo === 'documento') {
      msg.baixar = () => downloadMediaMessage(m, 'buffer', {}, { logger: log, reuploadRequest: sock.updateMediaMessage })
    }
    await assistente.mensagemDoCliente(msg)
  }

  return {
    envio,
    estado,
    conectar,
    usarAssistente (a) { assistente = a },
    jidDoTelefone,
    async pedirCodigoPareamento (telefone) {
      if (estado.situacao !== 'aguardando_pareamento') throw new Error('O WhatsApp não está esperando conexão.')
      return sock.requestPairingCode(soDigitos(telefone))
    },
    qrImagem: () => (estado.qr ? QRCode.toDataURL(estado.qr, { margin: 2, width: 320 }) : null)
  }
}
