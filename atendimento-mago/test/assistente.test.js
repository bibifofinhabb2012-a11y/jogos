import { test, beforeEach } from 'node:test'
import assert from 'node:assert/strict'
import fs from 'node:fs'
import os from 'node:os'
import path from 'node:path'
import { abrirBanco } from '../src/db.js'
import { criarAssistente } from '../src/assistente.js'
import { lerNascimento, lerValor, brl, reaisFalado, normalizarComando } from '../src/util.js'

const CLIENTE = '5551988887777@s.whatsapp.net'
let banco, bot, saida, relogio, pasta

function montar () {
  pasta = fs.mkdtempSync(path.join(os.tmpdir(), 'mago-'))
  banco = abrirBanco(':memory:')
  saida = []
  relogio = Date.parse('2026-10-31T15:00:00Z')
  const envio = {
    enviarTexto: async (jid, texto) => saida.push({ para: jid, texto }),
    enviarArquivo: async (jid, a) => saida.push({ para: jid, arquivo: a }),
    enviarTextoMago: async (texto) => saida.push({ para: 'MAGO', texto }),
    enviarArquivoMago: async (a) => saida.push({ para: 'MAGO', arquivo: a })
  }
  banco.salvarAjuste('pix_chave', '12345678900')
  banco.salvarAjuste('pix_titular', 'Fulano de Tal')
  bot = criarAssistente({ banco, envio, pastaArquivos: pasta, agora: () => relogio, log: { error: (e) => { throw e.err || e } } })
}
beforeEach(montar)

const texto = (t, jid = CLIENTE) => bot.mensagemDoCliente({ jid, tipo: 'texto', texto: t, nomeWhatsapp: 'Ana' })
const imagem = (jid = CLIENTE) => bot.mensagemDoCliente({ jid, tipo: 'imagem', mimetype: 'image/jpeg', baixar: async () => Buffer.from('jpg') })
const pdf = (jid = CLIENTE) => bot.mensagemDoCliente({ jid, tipo: 'documento', mimetype: 'application/pdf', baixar: async () => Buffer.from('pdf') })
const ultima = () => saida.at(-1)
const limpar = () => { saida = [] }

async function pedidoCompleto (opcoes, { comprovante = true, jid = CLIENTE } = {}) {
  await texto('oi', jid)
  for (const o of opcoes) await texto(String(o), jid)
  await texto('Ana Souza', jid)
  await texto('03/05/1990', jid)
  await imagem(jid)
  if (comprovante) await pdf(jid)
}

test('menu principal na primeira mensagem', async () => {
  await texto('Boa noite')
  const t = ultima().texto
  assert.match(t, /1\. Consultas\n2\. Trabalho espiritual\n3\. Falar com o Mago/)
  assert.match(t, /digite 0/)
})

test('cartas, jogo completo, primeira consulta: fluxo inteiro até o Mago', async () => {
  await texto('oi')
  await texto('1')
  assert.match(ultima().texto, /1\. Cartas\n2\. Búzios: R\$ 111\n3\. Dados de Zé Pilintra\n4\. Runas \(indisponível no momento\)/)
  await texto('1')
  assert.match(ultima().texto, /4\. Jogo completo: R\$ 130 na primeira consulta \(depois R\$ 150\)/)
  await texto('4')
  assert.match(ultima().texto, /Você escolheu: Cartas, jogo completo, R\$ 130 \(preço de primeira consulta\)/)
  await texto('Ana')
  assert.match(ultima().texto, /nome completo/)
  await texto('Ana Souza')
  assert.match(ultima().texto, /Obrigado, Ana/)
  await texto('31/02/1990')
  assert.match(ultima().texto, /Não consegui entender a data/)
  await texto('3 de maio de 1990')
  assert.match(ultima().texto, /foto sua/)
  await texto('segue')
  assert.match(ultima().texto, /Preciso de uma foto/)
  limpar()
  await imagem()
  assert.match(saida[0].texto, /Resumo do pedido número 1\n.*Cartas, jogo completo\nValor: R\$ 130 \(preço de primeira consulta\)\nNome: Ana Souza\nNascimento: 03\/05\/1990/s)
  assert.match(saida[0].texto, /Tipo de chave: CPF\nChave: 12345678900\nTitular: Fulano de Tal\nBanco: Nubank/)
  assert.equal(saida[1].texto, '12345678900')
  assert.equal(banco.pedido(1).situacao, 'aguardando_pagamento')

  limpar()
  await pdf()
  assert.match(saida[0].texto, /Comprovante recebido/)
  assert.equal(saida[1].para, 'MAGO')
  assert.equal(saida[1].texto, 'Novo pedido número 1. Nome: Ana Souza. Nascimento: 3 de maio de 1990. Serviço: Cartas, jogo completo, 130 reais, primeira consulta. Comprovante e foto a seguir.')
  assert.equal(saida[2].arquivo.mimetype, 'application/pdf')
  assert.equal(saida[3].arquivo.mimetype, 'image/jpeg')
  assert.ok(fs.existsSync(saida[3].arquivo.caminho))
  assert.equal(banco.pedido(1).situacao, 'conferir')

  // cliente escreve enquanto espera: um aviso só
  limpar()
  await texto('obrigada')
  await texto('?')
  assert.equal(saida.length, 1)
  assert.match(saida[0].texto, /já está com o Mago/)
})

test('pagou, cliente fica com o Mago, liberar devolve; segundo pedido sem desconto', async () => {
  await pedidoCompleto([1, 1, 4])
  limpar()
  assert.equal(await bot.comandoDoMago('Pagou 1.'), true)
  assert.match(saida[0].texto, /Pagamento confirmado! Seu pedido número 1/)
  assert.equal(saida[0].para, CLIENTE)
  assert.match(saida[1].texto, /Pedido 1 confirmado/)
  assert.equal(banco.pedido(1).situacao, 'com_mago')

  limpar()
  await texto('oi Mago')
  assert.equal(saida.length, 0, 'assistente quieto enquanto o Mago atende')

  await bot.comandoDoMago('liberar pedido 1')
  assert.equal(banco.pedido(1).situacao, 'concluido')
  limpar()
  await texto('oi')
  await texto('1')
  await texto('1')
  assert.match(ultima().texto, /4\. Jogo completo: R\$ 150$/m)
})

test('runas indisponível volta ao menu; áudio repete opções', async () => {
  await texto('oi')
  await texto('1')
  limpar()
  await texto('4')
  assert.match(saida[0].texto, /Runas está indisponível/)
  assert.match(saida[1].texto, /1\. Consultas/)
  limpar()
  await bot.mensagemDoCliente({ jid: CLIENTE, tipo: 'audio' })
  assert.match(saida[0].texto, /Não entendi/)
  assert.match(saida[1].texto, /1\. Consultas/)
  limpar()
  await texto('9')
  assert.match(saida[0].texto, /Não entendi/)
})

test('esconder item indisponível tira do menu e renumera', async () => {
  const runas = banco.todosItens().find(i => i.nome === 'Runas')
  banco.salvarItem(runas.id, { ...runas, esconder: 1 })
  await texto('oi')
  await texto('1')
  assert.doesNotMatch(ultima().texto, /Runas/)
})

test('0 cancela pedido sem comprovante', async () => {
  await pedidoCompleto([3], { comprovante: false })
  assert.equal(banco.pedido(1).servico, 'Falar com o Mago')
  await texto('0')
  assert.equal(banco.pedido(1).situacao, 'cancelado')
  assert.match(ultima().texto, /1\. Consultas/)
})

test('trabalho espiritual: taxa, valor depois, segundo comprovante', async () => {
  await texto('oi')
  await texto('2')
  assert.match(ultima().texto, /1\. Amor: valor sob consulta/)
  await texto('1')
  assert.match(ultima().texto, /Trabalho espiritual, amor, R\$ 10 para enviar o pedido/)
  await texto('Ana Souza')
  await texto('03051990')
  await imagem()
  limpar()
  await pdf()
  assert.match(saida[1].texto, /Serviço: Trabalho espiritual, amor, taxa de 10 reais, valor do trabalho sob consulta\./)

  await bot.comandoDoMago('pagou 1')
  limpar()
  await bot.comandoDoMago('valor 1 R$ 1.200,50')
  assert.match(saida[0].texto, /O valor do trabalho é R\$ 1\.200,50/)
  assert.equal(saida[1].texto, '12345678900')
  assert.match(saida[2].texto, /1200 reais e 50 centavos enviado/)
  limpar()
  await imagem()
  assert.match(saida[0].texto, /Comprovante recebido/)
  assert.match(saida[1].texto, /Comprovante do trabalho, pedido número 1\. Nome: Ana Souza\. Valor: 1200 reais e 50 centavos/)
  assert.equal(saida.length, 3)
  await bot.comandoDoMago('pagou 1')
  const p = banco.pedido(1)
  assert.equal(p.situacao, 'com_mago')
  assert.equal(p.trabalho_pago, 1)
})

test('valor em pedido que não é trabalho é recusado', async () => {
  await pedidoCompleto([3])
  limpar()
  await bot.comandoDoMago('valor 1 200')
  assert.match(ultima().texto, /não é de trabalho espiritual/)
})

test('contato ignorado e pausa quando o Mago escreve', async () => {
  const MAE = '5551911112222@s.whatsapp.net'
  banco.tocarCliente(MAE, 'Mãe', relogio)
  banco.marcarIgnorado(MAE, true)
  await texto('filho, liga pra mim', MAE)
  assert.equal(saida.length, 0)

  await bot.magoRespondeuCliente(CLIENTE)
  await texto('oi')
  assert.equal(saida.length, 0)
  relogio += 13 * 3600 * 1000
  await texto('oi')
  assert.match(ultima().texto, /1\. Consultas/)
})

test('conversa parada há mais de 24h recomeça', async () => {
  await texto('oi')
  await texto('1')
  await texto('2')
  assert.match(ultima().texto, /Búzios, R\$ 111/)
  relogio += 25 * 3600 * 1000
  await texto('Ana Souza')
  assert.match(ultima().texto, /1\. Consultas/)
})

test('pedidos de hoje, assumir, ajuda e texto que não é comando', async () => {
  await pedidoCompleto([1, 2])
  limpar()
  await bot.comandoDoMago('Pedidos de hoje')
  assert.equal(ultima().texto, '1 pedido hoje.\nPedido 1, Ana Souza, Búzios, pagamento para conferir.')
  await bot.comandoDoMago('assumir 1')
  assert.match(ultima().texto, /parou de responder/)
  assert.equal(banco.cliente(CLIENTE).com_mago, 1)
  await bot.comandoDoMago('pagou')
  assert.match(ultima().texto, /Faltou o número/)
  await bot.comandoDoMago('pagou 99')
  assert.match(ultima().texto, /Não achei o pedido 99/)
  limpar()
  assert.equal(await bot.comandoDoMago('lembrar de comprar vela'), false)
  assert.equal(saida.length, 0)
})

test('comandos renomeados no painel', async () => {
  banco.salvarAjuste('cmd_pagou', 'confirmar')
  await pedidoCompleto([3])
  limpar()
  await bot.comandoDoMago('Confirmar 1')
  assert.match(ultima().texto, /Pedido 1 confirmado/)
})

test('funções de data e dinheiro', () => {
  const agora = Date.parse('2026-10-31T12:00:00Z')
  assert.equal(lerNascimento('3/5/90', agora), '1990-05-03')
  assert.equal(lerNascimento('03.05.2001', agora), '2001-05-03')
  assert.equal(lerNascimento('1º de janeiro de 1980', agora), '1980-01-01')
  assert.equal(lerNascimento('3 mai 1990', agora), '1990-05-03')
  assert.equal(lerNascimento('29/02/2023', agora), null)
  assert.equal(lerNascimento('01/01/2030', agora), null)
  assert.equal(lerNascimento('ontem', agora), null)
  assert.equal(lerValor('1.200,5'), 120050)
  assert.equal(lerValor('R$ 200'), 20000)
  assert.equal(lerValor('abc'), null)
  assert.equal(brl(1500000), 'R$ 15.000')
  assert.equal(reaisFalado(100), '1 real')
  assert.equal(normalizarComando('Pagou, 12!'), 'pagou 12')
})

test('comando painel manda link só para número cadastrado, com ou sem o 9', async () => {
  const { criarLinkAcesso } = await import('../src/painel/acesso.js')
  banco.criarUsuario({ nome: 'Mago', telefone: '5551988880000' })
  const envio = { enviarTexto: async () => {}, enviarArquivo: async () => {}, enviarTextoMago: async (t) => saida.push({ para: 'MAGO', texto: t }), enviarArquivoMago: async () => {} }
  const linkPainel = async (tel) => { const u = banco.usuarioPorTelefone(tel); return u ? criarLinkAcesso(banco, u.id, 'https://x.trycloudflare.com') : null }
  const b = criarAssistente({ banco, envio, pastaArquivos: pasta, linkPainel })
  await b.comandoDoMago('Painel.', '555188880000')
  assert.match(ultima().texto, /^Link para entrar no painel\. Vale por 10 minutos:\nhttps:\/\/x\.trycloudflare\.com\/entrar\/link\/\S+$/)
  await b.comandoDoMago('painel', '5551977770000')
  assert.equal(ultima().texto, 'Este número não está cadastrado no painel.')
})
