import { test, before, after } from 'node:test'
import assert from 'node:assert/strict'
import fs from 'node:fs'
import os from 'node:os'
import path from 'node:path'
import { abrirBanco } from '../src/db.js'
import { criarAssistente } from '../src/assistente.js'
import { criarPainel } from '../src/painel/app.js'
import { hashSenha } from '../src/painel/acesso.js'

let servidor, base, banco, bot, enviadas
const log = { error: () => {}, info: () => {}, warn: () => {} }

before(async () => {
  const pasta = fs.mkdtempSync(path.join(os.tmpdir(), 'painel-'))
  banco = abrirBanco(':memory:')
  enviadas = []
  const envio = {
    enviarTexto: async (jid, texto) => enviadas.push({ jid, texto }),
    enviarArquivo: async () => {},
    enviarTextoMago: async (texto) => enviadas.push({ jid: 'MAGO', texto }),
    enviarArquivoMago: async () => {}
  }
  const whatsapp = {
    estado: { situacao: 'conectado', numero: '5551900000000' },
    envio,
    jidDoTelefone: async (t) => `${t}@s.whatsapp.net`,
    qrImagem: async () => null,
    pedirCodigoPareamento: async () => 'ABCD1234'
  }
  bot = criarAssistente({ banco, envio, pastaArquivos: pasta, log })
  banco.criarUsuario({ nome: 'Ricardo', usuario: 'ricardo', senha: hashSenha('senha-boa-123'), telefone: '5551999990000' })
  banco.criarUsuario({ nome: 'Mago', telefone: '5551988880000' })
  const app = criarPainel({ banco, assistente: bot, whatsapp, pastaArquivos: pasta, enderecoPublico: 'https://painel.teste', log })
  await new Promise(r => { servidor = app.listen(0, r) })
  base = `http://127.0.0.1:${servidor.address().port}`
})
after(() => servidor.close())

async function pedir (caminho, { cookie, form } = {}) {
  const r = await fetch(base + caminho, {
    method: form ? 'POST' : 'GET',
    redirect: 'manual',
    headers: { ...(cookie ? { cookie } : {}), ...(form ? { 'content-type': 'application/x-www-form-urlencoded' } : {}) },
    body: form ? new URLSearchParams(form).toString() : undefined
  })
  return { status: r.status, local: r.headers.get('location'), cookie: r.headers.get('set-cookie')?.split(';')[0], corpo: await r.text() }
}
async function seguir (r, cookie) {
  return r.local ? pedir(r.local, { cookie }) : r
}

test('sem login vai para a tela de entrar', async () => {
  const r = await pedir('/pedidos')
  assert.equal(r.status, 302)
  assert.equal(r.local, '/entrar')
  const t = await pedir('/entrar')
  assert.match(t.corpo, /<html lang="pt-BR">/)
  assert.match(t.corpo, /<label for="telefone">/)
})

test('senha errada e certa', async () => {
  const e = await pedir('/entrar/senha', { form: { usuario: 'ricardo', senha: 'errada' } })
  assert.equal(e.status, 401)
  assert.match(e.corpo, /role="alert"/)
  const ok = await pedir('/entrar/senha', { form: { usuario: 'Ricardo', senha: 'senha-boa-123' } })
  assert.equal(ok.status, 302)
  assert.match(ok.cookie, /^sessao=/)
  const inicio = await pedir('/', { cookie: ok.cookie })
  assert.match(inicio.corpo, /Olá, Ricardo/)
})

test('código pelo WhatsApp: vem primeiro na mensagem, errado conta tentativa', async () => {
  enviadas = []
  const r = await pedir('/entrar/whatsapp', { form: { telefone: '(51) 98888-0000', forma: 'codigo' } })
  assert.match(r.corpo, /Digite o código/)
  const msg = enviadas.at(-1)
  assert.equal(msg.jid, '5551988880000@s.whatsapp.net')
  const codigo = msg.texto.match(/^(\d{6}) é o seu código/)[1]
  const errado = await pedir('/entrar/codigo', { form: { telefone: '51988880000', codigo: codigo === '000000' ? '111111' : '000000' } })
  assert.match(errado.corpo, /Código errado/)
  const ok = await pedir('/entrar/codigo', { form: { telefone: '51988880000', codigo } })
  assert.equal(ok.status, 302)
  assert.match(ok.cookie, /^sessao=/)
  const denovo = await pedir('/entrar/codigo', { form: { telefone: '51988880000', codigo } })
  assert.match(denovo.corpo, /venceu ou não existe/)
})

test('link pelo WhatsApp entra uma vez só; número não cadastrado não recebe nada', async () => {
  enviadas = []
  await pedir('/entrar/whatsapp', { form: { telefone: '51988880000', forma: 'link' } })
  const url = enviadas.at(-1).texto.match(/https:\/\/painel\.teste(\/entrar\/link\/\S+)/)[1]
  const ok = await pedir(url)
  assert.equal(ok.status, 302)
  assert.match(ok.cookie, /^sessao=/)
  assert.equal((await pedir(url)).status, 410)

  enviadas = []
  const r = await pedir('/entrar/whatsapp', { form: { telefone: '51977770000', forma: 'link' } })
  assert.match(r.corpo, /Se este número estiver cadastrado/)
  assert.equal(enviadas.length, 0)
})

test('todas as páginas abrem e mudanças no painel valem no assistente', async () => {
  const { cookie } = await pedir('/entrar/senha', { form: { usuario: 'ricardo', senha: 'senha-boa-123' } })
  for (const p of ['/', '/pedidos', '/servicos', '/mensagens', '/mensagens/resumo', '/pix', '/contatos', '/ajustes', '/whatsapp', '/usuarios', '/conta']) {
    const r = await pedir(p, { cookie })
    assert.equal(r.status, 200, p)
    assert.match(r.corpo, /<h1>/, p)
    assert.match(r.corpo, /aria-current="page"/, p)
  }

  // Preço novo
  const cartas = banco.todosItens().find(i => i.nome === 'Pergunta única')
  let r = await pedir(`/servicos/${cartas.id}`, { cookie, form: { nome: 'Pergunta única', preco: '25', disponivel: 'on' } })
  r = await seguir(r, cookie)
  assert.match(r.corpo, /role="status" tabindex="-1">Salvo\. Pergunta única: R\$ 25\./)
  assert.match(r.corpo, /<title>Salvo\. Pergunta única/)

  // Runas disponível
  const runas = banco.todosItens().find(i => i.nome === 'Runas')
  await pedir(`/servicos/${runas.id}`, { cookie, form: { nome: 'Runas', titulo: 'Runas. Escolha:', no_nome: 'on', disponivel: 'on' } })
  // Búzios desce uma posição
  const buzios = banco.todosItens().find(i => i.nome === 'Búzios')
  await pedir(`/servicos/${buzios.id}/mover`, { cookie, form: { direcao: '1' } })

  // Pix e mensagem
  await pedir('/pix', { cookie, form: { pix_tipo: 'Celular', pix_chave: '51999990000', pix_titular: 'Fulano de Tal', pix_banco: 'Inter' } })
  await pedir('/mensagens/menu_principal', { cookie, form: { corpo: 'Bem-vindo ao Mago.\n{opcoes}' } })

  enviadas = []
  const J = '5551922223333@s.whatsapp.net'
  const t = (x) => bot.mensagemDoCliente({ jid: J, tipo: 'texto', texto: x })
  await t('oi')
  assert.equal(enviadas.at(-1).texto, 'Bem-vindo ao Mago.\n1. Consultas\n2. Trabalho espiritual\n3. Falar com o Mago: R$ 10')
  await t('1')
  assert.match(enviadas.at(-1).texto, /1\. Cartas\n2\. Dados de Zé Pilintra\n3\. Búzios: R\$ 111\n4\. Runas\n/)
  await t('1')
  assert.match(enviadas.at(-1).texto, /1\. Pergunta única: R\$ 25/)
  await t('1'); await t('Ana Souza'); await t('03/05/1990')
  await bot.mensagemDoCliente({ jid: J, tipo: 'imagem', mimetype: 'image/jpeg', baixar: async () => Buffer.from('x') })
  assert.match(enviadas.at(-2).texto, /Tipo de chave: Celular\nChave: 51999990000\nTitular: Fulano de Tal\nBanco: Inter/)

  // Pedido aparece no painel e a ação de confirmar funciona
  const id = banco.pedidos().at(0).id
  r = await pedir(`/pedidos/${id}`, { cookie })
  assert.match(r.corpo, /alt="Foto do pedido \d+, de Ana Souza"/)
  r = await seguir(await pedir(`/pedidos/${id}/acao`, { cookie, form: { acao: 'confirmarPagamento' } }), cookie)
  assert.match(r.corpo, /Pedido \d+ confirmado/)
  assert.equal(banco.pedido(id).situacao, 'com_mago')

  // Arquivo exige login
  assert.equal((await pedir(`/arquivo/${id}/foto`)).status, 302)
  assert.equal((await pedir(`/arquivo/${id}/foto`, { cookie })).status, 200)

  // Comandos repetidos são recusados
  r = await seguir(await pedir('/ajustes', { cookie, form: { pausa_horas: '12', limite_horas: '24', retencao_dias: '90', aviso_numero: '', cmd_pagou: 'ok', cmd_assumir: 'ok', cmd_liberar: 'liberar', cmd_valor: 'valor', cmd_hoje: 'hoje', cmd_ajuda: 'ajuda', cmd_painel: 'painel' } }), cookie)
  assert.match(r.corpo, /role="alert"[^>]*>Dois comandos/)

  // Escape de HTML
  await pedir('/mensagens/nao_entendi', { cookie, form: { corpo: '<script>alert(1)</script>' } })
  r = await pedir('/mensagens', { cookie })
  assert.doesNotMatch(r.corpo, /<script>alert/)
})
