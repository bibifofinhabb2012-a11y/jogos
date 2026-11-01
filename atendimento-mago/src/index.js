// Ponto de partida: abre o banco, conecta o WhatsApp, liga o assistente e o painel.
import fs from 'node:fs/promises'
import path from 'node:path'
import pino from 'pino'
import { abrirBanco } from './db.js'
import { criarAssistente } from './assistente.js'
import { criarWhatsapp } from './whatsapp.js'
import { criarPainel } from './painel/app.js'
import { criarLinkAcesso } from './painel/acesso.js'

const PORTA = Number(process.env.PORTA || 3000)
// HOST=127.0.0.1 deixa o painel visível só neste computador (o túnel continua funcionando).
const HOST = process.env.HOST || '0.0.0.0'
const PASTA_DADOS = path.resolve(process.env.PASTA_DADOS || 'dados')
const ENDERECO_PUBLICO = (process.env.ENDERECO_PUBLICO || `http://localhost:${PORTA}`).replace(/\/$/, '')

const log = pino({ level: process.env.LOG || 'info' })
const pastaArquivos = path.join(PASTA_DADOS, 'arquivos')
await fs.mkdir(pastaArquivos, { recursive: true })

const banco = abrirBanco(path.join(PASTA_DADOS, 'mago.db'))
if (banco.ajuste('pix_chave').startsWith('PREENCHER')) {
  log.warn('A chave Pix ainda não foi preenchida. Abra o painel, em Pix, antes de atender clientes.')
}
const whatsapp = criarWhatsapp({ pastaSessao: path.join(PASTA_DADOS, 'whatsapp-sessao'), banco, log })
const linkPainel = async (telefone) => {
  const u = banco.usuarioPorTelefone(telefone)
  return u ? criarLinkAcesso(banco, u.id, ENDERECO_PUBLICO) : null
}
const assistente = criarAssistente({ banco, envio: whatsapp.envio, pastaArquivos, linkPainel, log })
whatsapp.usarAssistente(assistente)

const painel = criarPainel({ banco, assistente, whatsapp, pastaArquivos, enderecoPublico: ENDERECO_PUBLICO, log })
painel.listen(PORTA, HOST, () => log.info(`Painel em ${ENDERECO_PUBLICO} (porta ${PORTA})`))

await whatsapp.conectar()

// De hora em hora: apaga fotos e comprovantes vencidos e acessos expirados.
async function limpeza () {
  try {
    const dias = Number(banco.ajuste('retencao_dias')) || 90
    const antes = Date.now() - dias * 24 * 3600 * 1000
    for (const p of banco.pedidosComArquivosAntes(antes)) {
      for (const campo of ['foto', 'comprovante', 'comprovante_trabalho']) {
        if (p[campo]) await fs.rm(path.join(pastaArquivos, path.basename(p[campo])), { force: true })
      }
      banco.atualizarPedido(p.id, { foto: null, comprovante: null, comprovante_trabalho: null }, Date.now())
      log.info({ pedido: p.id }, 'arquivos apagados pelo prazo de guarda')
    }
    banco.limparAcessos(Date.now())
  } catch (e) {
    log.error({ err: e }, 'falha na limpeza')
  }
}
await limpeza()
setInterval(limpeza, 3600 * 1000)
