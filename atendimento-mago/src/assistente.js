// O motor da conversa. Não sabe nada de WhatsApp: recebe mensagens já traduzidas
// e usa o "envio" (fornecido por quem o cria) para responder.
import fs from 'node:fs/promises'
import path from 'node:path'
import {
  preencher, brl, reaisFalado, lerNascimento, nascimentoExtenso, nascimentoCurto,
  normalizarComando, lerValor, diaLocal
} from './util.js'
import { SITUACOES } from './db.js'

const HORA = 3600 * 1000
const NUMEROS_ESCRITOS = { um: 1, uma: 1, dois: 2, duas: 2, tres: 3, quatro: 4, cinco: 5, seis: 6, sete: 7, oito: 8, nove: 9, zero: 0 }

const EXTENSOES = {
  'image/jpeg': '.jpg', 'image/png': '.png', 'image/webp': '.webp', 'image/heic': '.heic', 'application/pdf': '.pdf'
}

/**
 * @param {object} o
 * @param {ReturnType<import('./db.js').abrirBanco>} o.banco
 * @param {{enviarTexto, enviarArquivo, enviarTextoMago, enviarArquivoMago}} o.envio
 * @param {string} o.pastaArquivos  onde guardar fotos e comprovantes
 */
export function criarAssistente ({ banco, envio, pastaArquivos, agora = () => Date.now(), log = console }) {
  // Uma mensagem de cada vez, na ordem em que chegam. O volume é baixo e isso evita corridas.
  let fila = Promise.resolve()
  const emFila = (fn) => {
    const p = fila.then(fn)
    fila = p.catch((e) => log.error({ err: e }, 'erro no assistente'))
    return p
  }

  // ---------- textos ----------
  function varsPix () {
    return {
      pix_tipo: banco.ajuste('pix_tipo'),
      pix_chave: banco.ajuste('pix_chave'),
      pix_titular: banco.ajuste('pix_titular'),
      pix_banco: banco.ajuste('pix_banco')
    }
  }
  const montar = (chave, vars = {}) => preencher(banco.texto(chave), { ...varsPix(), ...vars })
  async function dizer (jid, chave, vars) {
    const t = montar(chave, vars)
    if (t.trim()) await envio.enviarTexto(jid, t)
  }
  const avisarMago = (texto) => envio.enviarTextoMago(texto)

  // ---------- serviços ----------
  function caminho (item) {
    const lista = []
    for (let it = item; it; it = it.pai_id == null ? null : banco.item(it.pai_id)) lista.unshift(it)
    return lista
  }
  function nomeServico (item) {
    const partes = caminho(item).filter(it => it.id === item.id || it.no_nome).map(it => it.nome)
    return partes.map((p, i) => (i === 0 ? p : p.charAt(0).toLowerCase() + p.slice(1))).join(', ')
  }
  function precoPara (item, jid) {
    const primeira = item.preco_primeira != null && !item.sob_consulta && !banco.jaPagou(jid)
    return { preco: primeira ? item.preco_primeira : (item.preco ?? 0), primeira }
  }
  function valorParaCliente (item, preco, primeira) {
    if (item.sob_consulta) return montar('valor_sob_consulta', { taxa: brl(preco) })
    return brl(preco) + (primeira ? montar('aviso_primeira') : '')
  }
  function linhaOpcao (item, jid) {
    if (!item.disponivel) return `${item.nome} (indisponível no momento)`
    if (item.tipo === 'menu') return item.nome
    if (item.sob_consulta) return `${item.nome}: valor sob consulta`
    const { preco, primeira } = precoPara(item, jid)
    if (primeira) return `${item.nome}: ${brl(preco)} na primeira consulta (depois ${brl(item.preco ?? 0)})`
    return `${item.nome}: ${brl(preco)}`
  }

  // ---------- estados do cliente ----------
  const salvar = (jid, estado) => banco.salvarEstado(jid, estado ? { ...estado, em: agora() } : null)

  async function mostrarMenu (jid, paiId = null) {
    const visiveis = banco.filhos(paiId).filter(it => it.disponivel || !it.esconder)
    const opcoes = visiveis.map((it, i) => `${i + 1}. ${linhaOpcao(it, jid)}`).join('\n')
    if (paiId == null) {
      await dizer(jid, 'menu_principal', { opcoes })
    } else {
      const pai = banco.item(paiId)
      await dizer(jid, 'submenu', { titulo: pai.titulo || `${pai.nome}. Escolha uma opção:`, opcoes })
    }
    salvar(jid, { passo: 'menu', pai: paiId, opcoes: visiveis.map(it => it.id) })
  }

  // Desiste do que estava em andamento. Pedido sem comprovante é cancelado.
  function abandonar (jid, estado) {
    if (estado?.pedido && estado.passo === 'comprovante' && !estado.trabalho) {
      const p = banco.pedido(estado.pedido)
      if (p?.situacao === 'aguardando_pagamento') banco.atualizarPedido(p.id, { situacao: 'cancelado' }, agora())
    }
    salvar(jid, null)
  }

  function lerOpcao (msg) {
    if (msg.tipo !== 'texto') return null
    const s = normalizarComando(msg.texto).replace(/^(opcao|numero|n) /, '')
    if (/^\d{1,2}$/.test(s)) return Number(s)
    return NUMEROS_ESCRITOS[s] ?? null
  }

  async function noMenu (jid, estado, msg) {
    const n = lerOpcao(msg)
    const id = n ? estado.opcoes[n - 1] : null
    const item = id ? banco.item(id) : null
    if (!item) {
      await dizer(jid, 'nao_entendi')
      return mostrarMenu(jid, estado.pai)
    }
    if (!item.disponivel) {
      await dizer(jid, 'indisponivel', { servico: item.nome })
      return mostrarMenu(jid, null)
    }
    if (item.tipo === 'menu') return mostrarMenu(jid, item.id)

    const { preco, primeira } = precoPara(item, jid)
    const rascunho = {
      item: item.id,
      servico: nomeServico(item),
      preco,
      primeira,
      sob_consulta: !!item.sob_consulta
    }
    await dizer(jid, 'pedir_nome', { servico: rascunho.servico, valor: valorParaCliente(item, preco, primeira) })
    salvar(jid, { passo: 'nome', rascunho })
  }

  async function noNome (jid, estado, msg) {
    const nome = msg.tipo === 'texto' ? msg.texto.replace(/\s+/g, ' ').trim() : ''
    const palavras = nome.split(' ').filter(p => /\p{L}/u.test(p))
    if (palavras.length < 2 || nome.length > 120 || /\d/.test(nome)) return dizer(jid, 'nome_invalido')
    estado.rascunho.nome = nome
    await dizer(jid, 'pedir_nascimento', { primeiro_nome: palavras[0] })
    salvar(jid, { ...estado, passo: 'nascimento' })
  }

  async function noNascimento (jid, estado, msg) {
    const iso = msg.tipo === 'texto' ? lerNascimento(msg.texto, agora()) : null
    if (!iso) return dizer(jid, 'nascimento_invalido')
    estado.rascunho.nascimento = iso
    await dizer(jid, 'pedir_foto', { dias: banco.ajuste('retencao_dias') })
    salvar(jid, { ...estado, passo: 'foto' })
  }

  const ehImagem = (msg) => msg.tipo === 'imagem' || (msg.tipo === 'documento' && /^image\//.test(msg.mimetype || ''))
  const ehComprovante = (msg) => ehImagem(msg) || (msg.tipo === 'documento' && msg.mimetype === 'application/pdf')

  async function guardarArquivo (msg, nomeBase) {
    const dados = await msg.baixar()
    const ext = EXTENSOES[msg.mimetype] || (msg.tipo === 'imagem' ? '.jpg' : '.bin')
    const arquivo = nomeBase + ext
    await fs.mkdir(pastaArquivos, { recursive: true })
    await fs.writeFile(path.join(pastaArquivos, arquivo), dados)
    return arquivo
  }

  async function noFoto (jid, estado, msg) {
    if (!ehImagem(msg)) return dizer(jid, 'foto_invalida')
    const r = estado.rascunho
    const id = banco.criarPedido({
      cliente_jid: jid, item_id: r.item, servico: r.servico, preco: r.preco,
      primeira: r.primeira, sob_consulta: r.sob_consulta, nome: r.nome, nascimento: r.nascimento
    }, agora())
    const foto = await guardarArquivo(msg, `${id}-foto`)
    banco.atualizarPedido(id, { foto }, agora())

    const item = banco.item(r.item)
    const valor = item ? valorParaCliente(item, r.preco, r.primeira) : brl(r.preco)
    await dizer(jid, 'resumo', {
      pedido: id, servico: r.servico, valor, nome: r.nome, nascimento: nascimentoCurto(r.nascimento)
    })
    await envio.enviarTexto(jid, banco.ajuste('pix_chave'))
    salvar(jid, { passo: 'comprovante', pedido: id })
  }

  async function noComprovante (jid, estado, msg) {
    if (!ehComprovante(msg)) return dizer(jid, 'comprovante_invalido')
    const p = banco.pedido(estado.pedido)
    if (!p || p.situacao === 'cancelado') {
      salvar(jid, null)
      return mostrarMenu(jid)
    }
    const trabalho = !!estado.trabalho
    const arquivo = await guardarArquivo(msg, `${p.id}-${trabalho ? 'comprovante-trabalho' : 'comprovante'}`)
    banco.atualizarPedido(p.id, { [trabalho ? 'comprovante_trabalho' : 'comprovante']: arquivo, situacao: 'conferir' }, agora())
    await dizer(jid, 'comprovante_recebido', { pedido: p.id })
    salvar(jid, { passo: 'esperando', pedido: p.id })
    await entregarAoMago(banco.pedido(p.id), trabalho)
  }

  function servicoParaMago (p) {
    if (p.sob_consulta) return `${p.servico}, taxa de ${reaisFalado(p.preco)}, valor do trabalho sob consulta`
    return `${p.servico}, ${reaisFalado(p.preco)}${p.primeira ? ', primeira consulta' : ''}`
  }

  const caminhoArquivo = (nome) => path.join(pastaArquivos, nome)
  const mimeDe = (nome) => Object.entries(EXTENSOES).find(([, e]) => nome.endsWith(e))?.[0] || 'application/octet-stream'

  async function entregarAoMago (p, trabalho) {
    if (trabalho) {
      await avisarMago(montar('mago_comprovante_trabalho', { pedido: p.id, nome: p.nome, valor: reaisFalado(p.valor_trabalho) }))
      await envio.enviarArquivoMago({ caminho: caminhoArquivo(p.comprovante_trabalho), mimetype: mimeDe(p.comprovante_trabalho), legenda: `Comprovante do trabalho, pedido ${p.id}.` })
      return
    }
    await avisarMago(montar('mago_novo_pedido', {
      pedido: p.id, nome: p.nome, nascimento: nascimentoExtenso(p.nascimento), servico: servicoParaMago(p)
    }))
    if (p.comprovante) await envio.enviarArquivoMago({ caminho: caminhoArquivo(p.comprovante), mimetype: mimeDe(p.comprovante), legenda: `Comprovante, pedido ${p.id}.` })
    if (p.foto) await envio.enviarArquivoMago({ caminho: caminhoArquivo(p.foto), mimetype: mimeDe(p.foto), legenda: `Foto de ${p.nome}, pedido ${p.id}.` })
  }

  async function esperando (jid, estado) {
    if (estado.avisado) return // já avisou uma vez; o Mago lê a conversa no próprio celular
    await dizer(jid, 'aguardando_mago', { pedido: estado.pedido })
    salvar(jid, { ...estado, avisado: true })
  }

  // ---------- entrada: mensagem de cliente ----------
  /**
   * msg: { jid, nomeWhatsapp, tipo: 'texto'|'imagem'|'documento'|'audio'|'outro', texto, mimetype, baixar: () => Promise<Buffer> }
   */
  function mensagemDoCliente (msg) {
    return emFila(async () => {
      const jid = msg.jid
      const c = banco.tocarCliente(jid, msg.nomeWhatsapp, agora())
      if (c.ignorado || c.com_mago) return
      if (c.pausado_ate && c.pausado_ate > agora()) return

      let estado = c.estado ? JSON.parse(c.estado) : null
      const limite = Number(banco.ajuste('limite_horas')) * HORA
      if (estado && ['menu', 'nome', 'nascimento', 'foto'].includes(estado.passo) && agora() - estado.em > limite) {
        abandonar(jid, estado)
        estado = null
      }

      if (msg.tipo === 'texto' && msg.texto.trim() === '0') {
        abandonar(jid, estado)
        return mostrarMenu(jid)
      }
      switch (estado?.passo) {
        case 'menu': return noMenu(jid, estado, msg)
        case 'nome': return noNome(jid, estado, msg)
        case 'nascimento': return noNascimento(jid, estado, msg)
        case 'foto': return noFoto(jid, estado, msg)
        case 'comprovante': return noComprovante(jid, estado, msg)
        case 'esperando': return esperando(jid, estado)
        default: return mostrarMenu(jid)
      }
    })
  }

  // ---------- entrada: o Mago escreveu na conversa de um cliente ----------
  function magoRespondeuCliente (jid) {
    return emFila(async () => {
      const c = banco.tocarCliente(jid, null, agora())
      if (c.com_mago) return
      const horas = Number(banco.ajuste('pausa_horas'))
      if (horas > 0) banco.pausar(jid, agora() + horas * HORA)
    })
  }

  // ---------- ações sobre pedidos (usadas pelos comandos e pelo painel) ----------
  // Cada uma retorna { ok, texto } com uma frase curta para o Mago ouvir.
  const acoes = {
    async confirmarPagamento (id) {
      const p = banco.pedido(id)
      if (!p) return { ok: false, texto: `Não achei o pedido ${id}.` }
      if (!['aguardando_pagamento', 'conferir'].includes(p.situacao)) {
        return { ok: false, texto: `O pedido ${id} está como ${SITUACOES[p.situacao].toLowerCase()}. Nada mudou.` }
      }
      const campos = { situacao: 'com_mago' }
      if (p.valor_trabalho) campos.trabalho_pago = 1
      banco.atualizarPedido(id, campos, agora())
      banco.marcarComMago(p.cliente_jid, true)
      salvar(p.cliente_jid, { passo: 'mago', pedido: id })
      await dizer(p.cliente_jid, 'pagamento_confirmado', { pedido: id })
      return { ok: true, texto: `Pedido ${id} confirmado. ${p.nome} foi avisado. O assistente parou de responder esse contato. Para devolver, diga ${banco.ajuste('cmd_liberar')} ${id}.` }
    },

    async assumir (id) {
      const p = banco.pedido(id)
      if (!p) return { ok: false, texto: `Não achei o pedido ${id}.` }
      banco.marcarComMago(p.cliente_jid, true)
      salvar(p.cliente_jid, { passo: 'mago', pedido: id })
      return { ok: true, texto: `Pedido ${id}, ${p.nome}: o assistente parou de responder. Pode conversar direto.` }
    },

    async liberar (id) {
      const p = banco.pedido(id)
      if (!p) return { ok: false, texto: `Não achei o pedido ${id}.` }
      banco.marcarComMago(p.cliente_jid, false)
      salvar(p.cliente_jid, null)
      if (p.situacao === 'com_mago') banco.atualizarPedido(id, { situacao: 'concluido' }, agora())
      await dizer(p.cliente_jid, 'atendimento_encerrado', { pedido: id })
      return { ok: true, texto: `Pedido ${id}, ${p.nome}: o assistente voltou a atender esse contato.` }
    },

    async informarValor (id, centavos) {
      const p = banco.pedido(id)
      if (!p) return { ok: false, texto: `Não achei o pedido ${id}.` }
      if (!p.sob_consulta) return { ok: false, texto: `O pedido ${id} não é de trabalho espiritual. Nada foi enviado.` }
      if (!centavos) return { ok: false, texto: `Faltou o valor. Exemplo: ${banco.ajuste('cmd_valor')} ${id} 200.` }
      banco.atualizarPedido(id, { valor_trabalho: centavos, trabalho_pago: 0, situacao: 'aguardando_pagamento' }, agora())
      banco.marcarComMago(p.cliente_jid, false)
      await dizer(p.cliente_jid, 'valor_trabalho', { pedido: id, valor: brl(centavos) })
      await envio.enviarTexto(p.cliente_jid, banco.ajuste('pix_chave'))
      salvar(p.cliente_jid, { passo: 'comprovante', pedido: id, trabalho: true })
      return { ok: true, texto: `Valor de ${reaisFalado(centavos)} enviado para ${p.nome}, pedido ${id}. O assistente vai pedir o comprovante.` }
    },

    async cancelar (id) {
      const p = banco.pedido(id)
      if (!p) return { ok: false, texto: `Não achei o pedido ${id}.` }
      banco.atualizarPedido(id, { situacao: 'cancelado' }, agora())
      const c = banco.cliente(p.cliente_jid)
      const est = c?.estado ? JSON.parse(c.estado) : null
      if (est?.pedido === id) {
        salvar(p.cliente_jid, null)
        banco.marcarComMago(p.cliente_jid, false)
      }
      return { ok: true, texto: `Pedido ${id} cancelado.` }
    },

    pedidosDeHoje () {
      const hoje = diaLocal(agora())
      const lista = banco.pedidosDesde(agora() - 36 * HORA).filter(p => diaLocal(p.criado_em) === hoje)
      if (!lista.length) return { ok: true, texto: 'Nenhum pedido hoje.' }
      const linhas = lista.map(p => `Pedido ${p.id}, ${p.nome}, ${p.servico}, ${SITUACOES[p.situacao].toLowerCase()}.`)
      return { ok: true, texto: `${lista.length} ${lista.length === 1 ? 'pedido' : 'pedidos'} hoje.\n` + linhas.join('\n') }
    }
  }

  // ---------- entrada: comando do Mago (conversa "Mensagem para mim" ou número de aviso) ----------
  function ajuda () {
    const a = banco.ajustes()
    return [
      'Comandos:',
      `${a.cmd_pagou} 12: confirma o pagamento do pedido 12 e avisa o cliente.`,
      `${a.cmd_assumir} 12: o assistente para de responder o cliente do pedido 12.`,
      `${a.cmd_liberar} 12: devolve o cliente ao assistente e conclui o pedido.`,
      `${a.cmd_valor} 12 200: envia ao cliente o valor de 200 reais do trabalho.`,
      `${a.cmd_hoje}: lista os pedidos do dia.`
    ].join('\n')
  }

  // Retorna true se reconheceu um comando. Texto que não é comando é ignorado
  // (o Mago pode usar a conversa consigo mesmo para anotações).
  function comandoDoMago (texto) {
    return emFila(async () => {
      const s = normalizarComando(texto)
      const a = banco.ajustes()
      const comandos = [
        ['pagou', a.cmd_pagou], ['assumir', a.cmd_assumir], ['liberar', a.cmd_liberar],
        ['valor', a.cmd_valor], ['hoje', a.cmd_hoje], ['ajuda', a.cmd_ajuda]
      ].map(([acao, palavra]) => [acao, normalizarComando(palavra)])
        .filter(([, p]) => p)
        .sort((x, y) => y[1].length - x[1].length)

      const achado = comandos.find(([, p]) => s === p || s.startsWith(p + ' '))
      if (!achado) return false
      const [acao, palavra] = achado
      const resto = s.slice(palavra.length).replace(/\b(pedido|numero|n|do|o|de|reais|real)\b/g, ' ').trim()
      const numeros = resto.match(/\d+(?:,\d{1,2})?/g) || []
      const id = numeros[0] && /^\d+$/.test(numeros[0]) ? Number(numeros[0]) : null

      let r
      if (acao === 'ajuda') r = { texto: ajuda() }
      else if (acao === 'hoje') r = acoes.pedidosDeHoje()
      else if (!id) r = { texto: `Faltou o número do pedido. Exemplo: ${palavra} 12.` }
      else if (acao === 'pagou') r = await acoes.confirmarPagamento(id)
      else if (acao === 'assumir') r = await acoes.assumir(id)
      else if (acao === 'liberar') r = await acoes.liberar(id)
      else if (acao === 'valor') r = await acoes.informarValor(id, numeros[1] ? lerValor(numeros[1]) : null)
      await avisarMago(r.texto)
      return true
    })
  }

  return {
    mensagemDoCliente,
    magoRespondeuCliente,
    comandoDoMago,
    // ações do painel passam pela mesma fila
    acoes: Object.fromEntries(Object.entries(acoes).map(([k, fn]) => [k, (...args) => emFila(() => fn(...args))])),
    servicoParaMago,
    nomeServico
  }
}
