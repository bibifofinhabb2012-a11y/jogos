// Banco de dados (SQLite, um arquivo só) e todas as consultas.
import { DatabaseSync } from 'node:sqlite'
import { TEXTOS } from './textos.js'
import { variantesTelefone } from './util.js'

const ESQUEMA = `
CREATE TABLE IF NOT EXISTS ajustes (chave TEXT PRIMARY KEY, valor TEXT NOT NULL);
CREATE TABLE IF NOT EXISTS textos (chave TEXT PRIMARY KEY, corpo TEXT NOT NULL);
CREATE TABLE IF NOT EXISTS itens (
  id INTEGER PRIMARY KEY,
  pai_id INTEGER REFERENCES itens(id) ON DELETE CASCADE,
  tipo TEXT NOT NULL CHECK (tipo IN ('menu', 'servico')),
  nome TEXT NOT NULL,
  titulo TEXT,                         -- menus: texto mostrado acima das opções
  no_nome INTEGER NOT NULL DEFAULT 1,  -- menus: entra no nome do serviço ("Cartas, jogo completo")
  preco INTEGER,                       -- centavos
  preco_primeira INTEGER,              -- centavos, preço para quem nunca fez pedido pago
  sob_consulta INTEGER NOT NULL DEFAULT 0,  -- 1 = preço é a taxa de envio; o Mago passa o valor depois
  disponivel INTEGER NOT NULL DEFAULT 1,
  esconder INTEGER NOT NULL DEFAULT 0,      -- 1 = some do menu quando indisponível
  posicao INTEGER NOT NULL DEFAULT 0
);
CREATE TABLE IF NOT EXISTS clientes (
  jid TEXT PRIMARY KEY,
  nome_whatsapp TEXT,
  ignorado INTEGER NOT NULL DEFAULT 0,      -- contato pessoal do Mago: o assistente nunca responde
  com_mago INTEGER NOT NULL DEFAULT 0,      -- o Mago assumiu: o assistente fica quieto
  pausado_ate INTEGER,                      -- o Mago escreveu na conversa: quieto até essa hora
  estado TEXT,
  visto_em INTEGER,
  criado_em INTEGER
);
CREATE TABLE IF NOT EXISTS pedidos (
  id INTEGER PRIMARY KEY,
  cliente_jid TEXT NOT NULL,
  item_id INTEGER,
  servico TEXT NOT NULL,          -- "Cartas, jogo completo"
  preco INTEGER NOT NULL,
  primeira INTEGER NOT NULL DEFAULT 0,
  sob_consulta INTEGER NOT NULL DEFAULT 0,
  valor_trabalho INTEGER,
  trabalho_pago INTEGER NOT NULL DEFAULT 0,
  nome TEXT NOT NULL,
  nascimento TEXT NOT NULL,
  foto TEXT,
  comprovante TEXT,
  comprovante_trabalho TEXT,
  situacao TEXT NOT NULL,
  criado_em INTEGER NOT NULL,
  atualizado_em INTEGER NOT NULL
);
CREATE INDEX IF NOT EXISTS pedidos_cliente ON pedidos (cliente_jid);
CREATE INDEX IF NOT EXISTS pedidos_criado ON pedidos (criado_em);
CREATE TABLE IF NOT EXISTS usuarios (
  id INTEGER PRIMARY KEY,
  nome TEXT NOT NULL,
  telefone TEXT UNIQUE,
  usuario TEXT UNIQUE,
  senha TEXT
);
CREATE TABLE IF NOT EXISTS acessos (
  hash TEXT PRIMARY KEY,
  usuario_id INTEGER NOT NULL REFERENCES usuarios(id) ON DELETE CASCADE,
  tipo TEXT NOT NULL,             -- link, codigo, sessao
  expira INTEGER NOT NULL,
  tentativas INTEGER NOT NULL DEFAULT 0
);
`

export const SITUACOES = {
  aguardando_pagamento: 'Aguardando pagamento',
  conferir: 'Pagamento para conferir',
  com_mago: 'Pago, com o Mago',
  concluido: 'Concluído',
  cancelado: 'Cancelado'
}

export const AJUSTES_PADRAO = {
  pix_tipo: 'CPF',
  pix_chave: 'PREENCHER NO PAINEL',
  pix_titular: 'PREENCHER NO PAINEL',
  pix_banco: 'Nubank',
  pausa_horas: '12',
  limite_horas: '24',
  retencao_dias: '90',
  aviso_numero: '',
  cmd_pagou: 'pagou',
  cmd_assumir: 'assumir',
  cmd_liberar: 'liberar',
  cmd_valor: 'valor',
  cmd_hoje: 'pedidos de hoje',
  cmd_ajuda: 'ajuda',
  cmd_painel: 'painel'
}

function semear (db) {
  const ins = db.prepare('INSERT OR IGNORE INTO ajustes (chave, valor) VALUES (?, ?)')
  for (const [k, v] of Object.entries(AJUSTES_PADRAO)) ins.run(k, v)
  const insT = db.prepare('INSERT OR IGNORE INTO textos (chave, corpo) VALUES (?, ?)')
  for (const t of TEXTOS) insT.run(t.chave, t.padrao)

  if (db.prepare('SELECT COUNT(*) AS n FROM itens').get().n > 0) return
  const add = (pai, tipo, nome, o = {}) => db.prepare(
    `INSERT INTO itens (pai_id, tipo, nome, titulo, no_nome, preco, preco_primeira, sob_consulta, disponivel, posicao)
     VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?)`
  ).run(pai, tipo, nome, o.titulo ?? null, o.no_nome ?? 1, o.preco ?? null, o.primeira ?? null,
    o.sob_consulta ? 1 : 0, o.disponivel === false ? 0 : 1, o.pos ?? 0).lastInsertRowid

  const consultas = add(null, 'menu', 'Consultas', { titulo: 'Consultas. Escolha o oráculo:', no_nome: 0, pos: 1 })
  const cartas = add(consultas, 'menu', 'Cartas', { titulo: 'Cartas. Escolha o tipo de jogo:', pos: 1 })
  add(cartas, 'servico', 'Pergunta única', { preco: 2000, pos: 1 })
  add(cartas, 'servico', 'Três perguntas', { preco: 5000, pos: 2 })
  add(cartas, 'servico', 'Análise amorosa', { preco: 7700, pos: 3 })
  add(cartas, 'servico', 'Jogo completo', { preco: 15000, primeira: 13000, pos: 4 })
  add(consultas, 'servico', 'Búzios', { preco: 11100, pos: 2 })
  const dados = add(consultas, 'menu', 'Dados de Zé Pilintra', { titulo: 'Dados de Zé Pilintra. Escolha o tipo de jogo:', pos: 3 })
  add(dados, 'servico', 'Pergunta única', { preco: 3500, pos: 1 })
  add(dados, 'servico', 'Três perguntas', { preco: 5000, pos: 2 })
  add(consultas, 'menu', 'Runas', { disponivel: false, pos: 4 })
  const trabalho = add(null, 'menu', 'Trabalho espiritual', { titulo: 'Trabalho espiritual. Escolha a linha:', pos: 2 })
  ;['Amor', 'Carreira', 'Limpezas', 'Outros'].forEach((n, i) =>
    add(trabalho, 'servico', n, { preco: 1000, sob_consulta: true, pos: i + 1 }))
  add(null, 'servico', 'Falar com o Mago', { preco: 1000, pos: 3 })
}

export function abrirBanco (arquivo) {
  const db = new DatabaseSync(arquivo)
  db.exec('PRAGMA journal_mode = WAL; PRAGMA foreign_keys = ON; PRAGMA busy_timeout = 5000;')
  db.exec(ESQUEMA)
  semear(db)
  return criarConsultas(db)
}

function criarConsultas (db) {
  const q = (sql) => db.prepare(sql)
  return {
    db,

    // Ajustes e textos
    ajuste: (k) => q('SELECT valor FROM ajustes WHERE chave = ?').get(k)?.valor ?? AJUSTES_PADRAO[k] ?? '',
    ajustes: () => Object.fromEntries(q('SELECT chave, valor FROM ajustes').all().map(r => [r.chave, r.valor])),
    salvarAjuste: (k, v) => q('INSERT INTO ajustes (chave, valor) VALUES (?, ?) ON CONFLICT(chave) DO UPDATE SET valor = excluded.valor').run(k, String(v)),
    texto: (k) => q('SELECT corpo FROM textos WHERE chave = ?').get(k)?.corpo ?? '',
    salvarTexto: (k, v) => q('INSERT INTO textos (chave, corpo) VALUES (?, ?) ON CONFLICT(chave) DO UPDATE SET corpo = excluded.corpo').run(k, v),

    // Serviços
    item: (id) => q('SELECT * FROM itens WHERE id = ?').get(id),
    filhos: (paiId) => paiId == null
      ? q('SELECT * FROM itens WHERE pai_id IS NULL ORDER BY posicao, id').all()
      : q('SELECT * FROM itens WHERE pai_id = ? ORDER BY posicao, id').all(paiId),
    todosItens: () => q('SELECT * FROM itens ORDER BY posicao, id').all(),
    criarItem (paiId, tipo, nome) {
      const pos = (paiId == null
        ? q('SELECT MAX(posicao) AS p FROM itens WHERE pai_id IS NULL').get()
        : q('SELECT MAX(posicao) AS p FROM itens WHERE pai_id = ?').get(paiId)).p ?? 0
      return Number(q('INSERT INTO itens (pai_id, tipo, nome, posicao) VALUES (?, ?, ?, ?)').run(paiId, tipo, nome, pos + 1).lastInsertRowid)
    },
    salvarItem: (id, c) => q(`UPDATE itens SET nome = ?, titulo = ?, no_nome = ?, preco = ?, preco_primeira = ?,
        sob_consulta = ?, disponivel = ?, esconder = ? WHERE id = ?`)
      .run(c.nome, c.titulo, c.no_nome, c.preco, c.preco_primeira, c.sob_consulta, c.disponivel, c.esconder, id),
    excluirItem: (id) => q('DELETE FROM itens WHERE id = ?').run(id),
    // Troca de lugar com o vizinho de cima (-1) ou de baixo (+1). Retorna false se já está na ponta.
    moverItem (id, direcao) {
      const it = q('SELECT * FROM itens WHERE id = ?').get(id)
      if (!it) return false
      const irmaos = this.filhos(it.pai_id)
      const i = irmaos.findIndex(x => x.id === id)
      const j = i + direcao
      if (j < 0 || j >= irmaos.length) return false
      ;[irmaos[i], irmaos[j]] = [irmaos[j], irmaos[i]]
      const up = q('UPDATE itens SET posicao = ? WHERE id = ?')
      irmaos.forEach((x, k) => up.run(k + 1, x.id))
      return true
    },

    // Clientes
    cliente: (jid) => q('SELECT * FROM clientes WHERE jid = ?').get(jid),
    tocarCliente (jid, nomeWhatsapp, agora) {
      q(`INSERT INTO clientes (jid, nome_whatsapp, visto_em, criado_em) VALUES (?, ?, ?, ?)
         ON CONFLICT(jid) DO UPDATE SET visto_em = excluded.visto_em,
           nome_whatsapp = COALESCE(excluded.nome_whatsapp, clientes.nome_whatsapp)`)
        .run(jid, nomeWhatsapp || null, agora, agora)
      return q('SELECT * FROM clientes WHERE jid = ?').get(jid)
    },
    salvarEstado: (jid, estado) => q('UPDATE clientes SET estado = ? WHERE jid = ?').run(estado ? JSON.stringify(estado) : null, jid),
    marcarComMago: (jid, sim) => q('UPDATE clientes SET com_mago = ?, pausado_ate = NULL WHERE jid = ?').run(sim ? 1 : 0, jid),
    pausar: (jid, ate) => q('UPDATE clientes SET pausado_ate = ? WHERE jid = ?').run(ate, jid),
    marcarIgnorado: (jid, sim) => q('UPDATE clientes SET ignorado = ? WHERE jid = ?').run(sim ? 1 : 0, jid),
    contatos: () => q(`SELECT c.*, (SELECT COUNT(*) FROM pedidos p WHERE p.cliente_jid = c.jid) AS pedidos
                       FROM clientes c ORDER BY c.ignorado DESC, c.visto_em DESC LIMIT 300`).all(),
    jaPagou: (jid) => q(`SELECT COUNT(*) AS n FROM pedidos WHERE cliente_jid = ? AND situacao IN ('com_mago', 'concluido')`).get(jid).n > 0,

    // Pedidos
    pedido: (id) => q('SELECT * FROM pedidos WHERE id = ?').get(id),
    criarPedido (p, agora) {
      return Number(q(`INSERT INTO pedidos (cliente_jid, item_id, servico, preco, primeira, sob_consulta, nome, nascimento, situacao, criado_em, atualizado_em)
        VALUES (?, ?, ?, ?, ?, ?, ?, ?, 'aguardando_pagamento', ?, ?)`)
        .run(p.cliente_jid, p.item_id, p.servico, p.preco, p.primeira ? 1 : 0, p.sob_consulta ? 1 : 0, p.nome, p.nascimento, agora, agora).lastInsertRowid)
    },
    atualizarPedido (id, campos, agora) {
      const ks = Object.keys(campos)
      q(`UPDATE pedidos SET ${ks.map(k => `${k} = ?`).join(', ')}, atualizado_em = ? WHERE id = ?`)
        .run(...ks.map(k => campos[k]), agora, id)
    },
    pedidos (situacao) {
      return situacao
        ? q('SELECT * FROM pedidos WHERE situacao = ? ORDER BY id DESC LIMIT 200').all(situacao)
        : q('SELECT * FROM pedidos ORDER BY id DESC LIMIT 200').all()
    },
    pedidosDesde: (ts) => q('SELECT * FROM pedidos WHERE criado_em >= ? ORDER BY id').all(ts),
    contarPorSituacao: () => Object.fromEntries(q('SELECT situacao, COUNT(*) AS n FROM pedidos GROUP BY situacao').all().map(r => [r.situacao, r.n])),
    pedidosComArquivosAntes: (ts) => q(`SELECT * FROM pedidos WHERE criado_em < ?
      AND (foto IS NOT NULL OR comprovante IS NOT NULL OR comprovante_trabalho IS NOT NULL)`).all(ts),

    // Usuários do painel e acessos
    usuario: (id) => q('SELECT * FROM usuarios WHERE id = ?').get(id),
    usuarioPorTelefone (tel) {
      for (const v of variantesTelefone(tel)) {
        const u = q('SELECT * FROM usuarios WHERE telefone = ?').get(v)
        if (u) return u
      }
    },
    usuarioPorLogin: (u) => q('SELECT * FROM usuarios WHERE usuario = ?').get(u),
    usuarios: () => q('SELECT * FROM usuarios ORDER BY nome').all(),
    criarUsuario: (u) => Number(q('INSERT INTO usuarios (nome, telefone, usuario, senha) VALUES (?, ?, ?, ?)')
      .run(u.nome, u.telefone || null, u.usuario || null, u.senha || null).lastInsertRowid),
    salvarUsuario: (id, u) => q('UPDATE usuarios SET nome = ?, telefone = ?, usuario = ? WHERE id = ?').run(u.nome, u.telefone || null, u.usuario || null, id),
    salvarSenha: (id, hash) => q('UPDATE usuarios SET senha = ? WHERE id = ?').run(hash, id),
    excluirUsuario: (id) => q('DELETE FROM usuarios WHERE id = ?').run(id),
    criarAcesso: (hash, usuarioId, tipo, expira) => q('INSERT INTO acessos (hash, usuario_id, tipo, expira) VALUES (?, ?, ?, ?)').run(hash, usuarioId, tipo, expira),
    acesso: (hash) => q('SELECT * FROM acessos WHERE hash = ?').get(hash),
    codigoAtivo: (usuarioId, agora) => q(`SELECT * FROM acessos WHERE usuario_id = ? AND tipo = 'codigo' AND expira > ? ORDER BY expira DESC LIMIT 1`).get(usuarioId, agora),
    somarTentativa: (hash) => q('UPDATE acessos SET tentativas = tentativas + 1 WHERE hash = ?').run(hash),
    apagarAcesso: (hash) => q('DELETE FROM acessos WHERE hash = ?').run(hash),
    apagarCodigos: (usuarioId) => q(`DELETE FROM acessos WHERE usuario_id = ? AND tipo IN ('codigo', 'link')`).run(usuarioId),
    limparAcessos: (agora) => q('DELETE FROM acessos WHERE expira < ?').run(agora)
  }
}
