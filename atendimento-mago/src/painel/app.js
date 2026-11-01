// Painel de administração.
import path from 'node:path'
import crypto from 'node:crypto'
import express from 'express'
import { SITUACOES, AJUSTES_PADRAO } from '../db.js'
import { TEXTOS, TEXTOS_POR_CHAVE } from '../textos.js'
import { brl, lerValor, dataHoraCurta, nascimentoCurto, normalizarTelefone, telefoneDoJid } from '../util.js'
import { pagina, html, cru, campo, caixaMarcar } from './html.js'
import { montarAcesso, hashSenha } from './acesso.js'

export function criarPainel ({ banco, assistente, whatsapp, pastaArquivos, enderecoPublico, log }) {
  const app = express()
  app.set('trust proxy', 1)
  app.disable('x-powered-by')
  app.use(express.urlencoded({ extended: false, limit: '200kb' }))
  app.use((req, res, next) => {
    res.setHeader('X-Frame-Options', 'DENY')
    res.setHeader('Referrer-Policy', 'no-referrer')
    res.setHeader('Cache-Control', 'no-store')
    req.log = log
    next()
  })

  montarAcesso({ app, banco, whatsapp, enderecoPublico })

  // ---------- avisos depois de salvar (lidos em voz alta na página seguinte) ----------
  const avisos = new Map()
  function voltar (res, url, texto, tipo = 'ok') {
    const id = crypto.randomBytes(6).toString('hex')
    avisos.set(id, { texto, tipo, em: Date.now() })
    for (const [k, v] of avisos) if (Date.now() - v.em > 10 * 60 * 1000) avisos.delete(k)
    res.redirect(url + (url.includes('?') ? '&' : '?') + 'aviso=' + id)
  }
  function mostrar (req, res, titulo, corpo, caminho) {
    const aviso = avisos.get(req.query.aviso)
    if (aviso) avisos.delete(req.query.aviso)
    res.send(pagina({ titulo, corpo, usuario: req.usuario, aviso, caminho }))
  }
  const marcado = (v) => (v === 'on' ? 1 : 0)
  const idDe = (req) => Number(req.params.id)

  // ---------- início ----------
  app.get('/', (req, res) => {
    const n = banco.contarPorSituacao()
    const zap = { conectado: 'conectado', aguardando_pareamento: 'esperando conexão', reconectando: 'reconectando', desconectado: 'desconectado', iniciando: 'iniciando' }[whatsapp.estado.situacao]
    mostrar(req, res, `Olá, ${req.usuario.nome}`, html`
      <p>WhatsApp do assistente: ${zap}. ${whatsapp.estado.situacao !== 'conectado' ? html`<a href="/whatsapp">Ver conexão</a>` : ''}</p>
      <h2>Pedidos</h2>
      <ul class="lista">
        ${Object.entries(SITUACOES).map(([k, rotulo]) => html`<li><a href="/pedidos?situacao=${k}">${rotulo}: ${n[k] || 0}</a></li>`)}
      </ul>`, '/')
  })

  // ---------- pedidos ----------
  app.get('/pedidos', (req, res) => {
    const filtro = SITUACOES[req.query.situacao] ? req.query.situacao : ''
    const lista = banco.pedidos(filtro)
    const opcoes = [['', 'Todos'], ...Object.entries(SITUACOES)]
    mostrar(req, res, filtro ? `Pedidos: ${SITUACOES[filtro]}` : 'Pedidos', html`
      <nav aria-label="Filtrar pedidos"><p>Mostrar:
        ${opcoes.map(([k, r], i) => html`${i ? ' · ' : ''}<a href="/pedidos${k ? '?situacao=' + k : ''}"${cru(k === filtro ? ' aria-current="true"' : '')}>${r}</a>`)}
      </p></nav>
      ${lista.length ? '' : html`<p>Nenhum pedido.</p>`}
      <ul class="lista">
        ${lista.map(p => html`<li>
          <a href="/pedidos/${p.id}"><strong>Pedido ${p.id}, ${p.nome}</strong></a><br>
          ${p.servico}, ${brl(p.preco)}${p.valor_trabalho ? html`, trabalho ${brl(p.valor_trabalho)}` : ''}.<br>
          ${SITUACOES[p.situacao]}. ${dataHoraCurta(p.criado_em)}.
        </li>`)}
      </ul>`, '/pedidos')
  })

  app.get('/pedidos/:id', (req, res) => {
    const p = banco.pedido(idDe(req))
    if (!p) return voltar(res, '/pedidos', 'Pedido não encontrado.', 'erro')
    const anexo = (qual, rotulo) => {
      const arq = p[qual]
      if (!arq) return html`<dd>Não enviado, ou já apagado pelo prazo de guarda.</dd>`
      const url = `/arquivo/${p.id}/${qual}`
      if (arq.endsWith('.pdf')) return html`<dd><a href="${url}">Abrir ${rotulo.toLowerCase()} (PDF)</a></dd>`
      return html`<dd><a href="${url}">Abrir ${rotulo.toLowerCase()} em tela cheia</a><br><img class="anexo" src="${url}" alt="${rotulo} do pedido ${p.id}, de ${p.nome}"></dd>`
    }
    const tel = telefoneDoJid(p.cliente_jid)
    mostrar(req, res, `Pedido ${p.id}, ${p.nome}`, html`
      <dl>
        <dt>Situação</dt><dd>${SITUACOES[p.situacao]}</dd>
        <dt>Serviço</dt><dd>${p.servico}</dd>
        <dt>Valor</dt><dd>${brl(p.preco)}${p.primeira ? ', preço de primeira consulta' : ''}${p.sob_consulta ? ', taxa de envio' : ''}</dd>
        ${p.sob_consulta ? html`<dt>Valor do trabalho</dt><dd>${p.valor_trabalho ? html`${brl(p.valor_trabalho)}, ${p.trabalho_pago ? 'pago' : 'não pago ainda'}` : 'Ainda não informado'}</dd>` : ''}
        <dt>Nome</dt><dd>${p.nome}</dd>
        <dt>Nascimento</dt><dd>${nascimentoCurto(p.nascimento)}</dd>
        <dt>WhatsApp</dt><dd>${tel ? html`<a href="https://wa.me/${tel}">${tel}</a>` : p.cliente_jid}</dd>
        <dt>Feito em</dt><dd>${dataHoraCurta(p.criado_em)}</dd>
        <dt>Comprovante</dt>${anexo('comprovante', 'Comprovante')}
        ${p.sob_consulta ? html`<dt>Comprovante do trabalho</dt>${anexo('comprovante_trabalho', 'Comprovante do trabalho')}` : ''}
        <dt>Foto</dt>${anexo('foto', 'Foto')}
      </dl>
      <h2>Ações</h2>
      <form method="post" action="/pedidos/${p.id}/acao">
        ${['aguardando_pagamento', 'conferir'].includes(p.situacao) ? html`<button name="acao" value="confirmarPagamento">Confirmar pagamento e avisar o cliente</button>` : ''}
        <button name="acao" value="assumir" class="secundario">Assumir: o assistente para de responder</button>
        <button name="acao" value="liberar" class="secundario">Liberar: o assistente volta a responder</button>
        ${p.situacao !== 'cancelado' ? html`<button name="acao" value="cancelar" class="secundario">Cancelar pedido</button>` : ''}
      </form>
      ${p.sob_consulta && p.situacao !== 'cancelado' ? html`
        <h2>Valor do trabalho</h2>
        <form method="post" action="/pedidos/${p.id}/valor">
          ${campo({ nome: 'valor', rotulo: 'Valor do trabalho, em reais', ajuda: 'O cliente recebe o valor e os dados do Pix, e o assistente pede o comprovante.', obrigatorio: true, extra: 'inputmode="decimal"' })}
          <button type="submit">Enviar valor ao cliente</button>
        </form>` : ''}
      <p><a href="/pedidos">Voltar aos pedidos</a></p>`, '/pedidos')
  })

  app.post('/pedidos/:id/acao', async (req, res) => {
    const id = idDe(req)
    const fn = { confirmarPagamento: 1, assumir: 1, liberar: 1, cancelar: 1 }[req.body.acao] && assistente.acoes[req.body.acao]
    if (!fn) return voltar(res, `/pedidos/${id}`, 'Ação desconhecida.', 'erro')
    try {
      const r = await fn(id)
      voltar(res, `/pedidos/${id}`, r.texto, r.ok ? 'ok' : 'erro')
    } catch (e) {
      log.error({ err: e }, 'falha na ação do painel')
      voltar(res, `/pedidos/${id}`, 'Não consegui avisar o cliente. Confira se o WhatsApp está conectado.', 'erro')
    }
  })

  app.post('/pedidos/:id/valor', async (req, res) => {
    const id = idDe(req)
    const valor = lerValor(req.body.valor || '')
    if (!valor) return voltar(res, `/pedidos/${id}`, 'Valor inválido. Escreva só o número, por exemplo 200 ou 200,50.', 'erro')
    try {
      const r = await assistente.acoes.informarValor(id, valor)
      voltar(res, `/pedidos/${id}`, r.texto, r.ok ? 'ok' : 'erro')
    } catch (e) {
      log.error({ err: e }, 'falha ao enviar valor')
      voltar(res, `/pedidos/${id}`, 'Não consegui avisar o cliente. Confira se o WhatsApp está conectado.', 'erro')
    }
  })

  app.get('/arquivo/:id/:qual', (req, res) => {
    const p = banco.pedido(idDe(req))
    const qual = ['foto', 'comprovante', 'comprovante_trabalho'].includes(req.params.qual) ? req.params.qual : null
    if (!p || !qual || !p[qual]) return res.status(404).send('Arquivo não encontrado.')
    res.sendFile(path.join(pastaArquivos, path.basename(p[qual])))
  })

  // ---------- serviços ----------
  function resumoItem (it) {
    const partes = []
    if (it.tipo === 'menu') partes.push('menu')
    else if (it.sob_consulta) partes.push(`sob consulta, taxa ${brl(it.preco ?? 0)}`)
    else {
      partes.push(brl(it.preco ?? 0))
      if (it.preco_primeira != null) partes.push(`primeira consulta ${brl(it.preco_primeira)}`)
    }
    if (!it.disponivel) partes.push(it.esconder ? 'indisponível, escondido' : 'indisponível')
    return partes.join(', ')
  }
  function arvore (paiId) {
    const filhos = banco.filhos(paiId)
    if (!filhos.length) return ''
    return html`<ul class="arvore">${filhos.map(it => html`<li><a href="/servicos/${it.id}">${it.nome}</a>: ${resumoItem(it)}${cru(arvore(it.id).toString())}</li>`)}</ul>`
  }
  const formNovo = (paiId, rotuloOnde) => html`
    <h2>Adicionar ${rotuloOnde}</h2>
    <form method="post" action="/servicos/novo">
      <input type="hidden" name="pai" value="${paiId ?? ''}">
      ${campo({ nome: 'nome', rotulo: 'Nome', obrigatorio: true })}
      <fieldset><legend>Tipo</legend>
        <div class="marcar"><input type="radio" id="tipo-servico" name="tipo" value="servico" checked><label for="tipo-servico" style="margin:0">Serviço, com preço</label></div>
        <div class="marcar"><input type="radio" id="tipo-menu" name="tipo" value="menu"><label for="tipo-menu" style="margin:0">Menu, com outras opções dentro</label></div>
      </fieldset>
      <button type="submit">Adicionar</button>
    </form>`

  app.get('/servicos', (req, res) => {
    mostrar(req, res, 'Serviços e preços', html`
      <p>Os itens aparecem para o cliente nesta ordem. Toque num item para mudar nome, preço, ordem ou disponibilidade.</p>
      <h2>Menu principal</h2>
      ${cru(arvore(null).toString())}
      ${formNovo(null, 'ao menu principal')}`, '/servicos')
  })

  app.post('/servicos/novo', (req, res) => {
    const nome = String(req.body.nome || '').trim()
    const pai = req.body.pai ? Number(req.body.pai) : null
    if (!nome) return voltar(res, pai ? `/servicos/${pai}` : '/servicos', 'Escreva o nome.', 'erro')
    if (pai && banco.item(pai)?.tipo !== 'menu') return voltar(res, '/servicos', 'Só dá para adicionar dentro de um menu.', 'erro')
    const tipo = req.body.tipo === 'menu' ? 'menu' : 'servico'
    const id = banco.criarItem(pai, tipo, nome)
    voltar(res, `/servicos/${id}`, tipo === 'servico' ? `${nome} adicionado. Agora defina o preço.` : `${nome} adicionado.`)
  })

  app.get('/servicos/:id', (req, res) => {
    const it = banco.item(idDe(req))
    if (!it) return voltar(res, '/servicos', 'Item não encontrado.', 'erro')
    const caminho = []
    for (let x = it.pai_id == null ? null : banco.item(it.pai_id); x; x = x.pai_id == null ? null : banco.item(x.pai_id)) caminho.unshift(x)
    const reais = (c) => (c == null ? '' : (c / 100).toFixed(2).replace('.', ','))
    mostrar(req, res, it.nome, html`
      <p>Fica em: Menu principal${caminho.map(x => html` › <a href="/servicos/${x.id}">${x.nome}</a>`)}. ${it.tipo === 'menu' ? 'Tipo: menu.' : 'Tipo: serviço.'}</p>
      <form method="post" action="/servicos/${it.id}">
        ${campo({ nome: 'nome', rotulo: 'Nome', valor: it.nome, obrigatorio: true })}
        ${it.tipo === 'menu'
          ? html`
            ${campo({ nome: 'titulo', rotulo: 'Texto acima das opções', valor: it.titulo || '', ajuda: 'Por exemplo: Cartas. Escolha o tipo de jogo:' })}
            ${caixaMarcar({ nome: 'no_nome', rotulo: 'Entra no nome do serviço', marcado: it.no_nome, ajuda: 'Marcado, o pedido aparece como "Cartas, jogo completo". Desmarcado, este nome fica de fora, como acontece com Consultas.' })}`
          : html`
            ${campo({ nome: 'preco', rotulo: 'Preço, em reais', valor: reais(it.preco), obrigatorio: true, extra: 'inputmode="decimal"' })}
            ${campo({ nome: 'preco_primeira', rotulo: 'Preço na primeira consulta, em reais', valor: reais(it.preco_primeira), ajuda: 'Deixe em branco se não tem desconto. Vale para o número que nunca teve um pedido pago.', extra: 'inputmode="decimal"' })}
            ${caixaMarcar({ nome: 'sob_consulta', rotulo: 'Valor sob consulta', marcado: it.sob_consulta, ajuda: 'Marcado, o preço acima é a taxa para enviar o pedido. O Mago informa o valor do trabalho depois.' })}`}
        ${caixaMarcar({ nome: 'disponivel', rotulo: 'Disponível', marcado: it.disponivel })}
        ${caixaMarcar({ nome: 'esconder', rotulo: 'Esconder do menu quando indisponível', marcado: it.esconder, ajuda: 'Desmarcado, o item aparece com o aviso "indisponível no momento".' })}
        <button type="submit">Salvar</button>
      </form>
      <h2>Ordem no menu</h2>
      <form method="post" action="/servicos/${it.id}/mover">
        <button name="direcao" value="-1" class="secundario">Subir uma posição</button>
        <button name="direcao" value="1" class="secundario">Descer uma posição</button>
      </form>
      ${it.tipo === 'menu' ? html`
        <h2>Opções dentro de ${it.nome}</h2>
        ${banco.filhos(it.id).length ? cru(arvore(it.id).toString()) : html`<p>Nenhuma opção ainda.</p>`}
        ${formNovo(it.id, `dentro de ${it.nome}`)}` : ''}
      <h2>Excluir</h2>
      <form method="get" action="/servicos/${it.id}/excluir">
        <button type="submit" class="secundario">Excluir ${it.nome}${it.tipo === 'menu' ? ' e tudo que está dentro' : ''}</button>
      </form>
      <p><a href="${it.pai_id ? `/servicos/${it.pai_id}` : '/servicos'}">Voltar</a></p>`, '/servicos')
  })

  app.post('/servicos/:id', (req, res) => {
    const it = banco.item(idDe(req))
    if (!it) return voltar(res, '/servicos', 'Item não encontrado.', 'erro')
    const b = req.body
    const nome = String(b.nome || '').trim()
    if (!nome) return voltar(res, `/servicos/${it.id}`, 'O nome não pode ficar vazio.', 'erro')
    const novo = { ...it, nome, disponivel: marcado(b.disponivel), esconder: marcado(b.esconder) }
    if (it.tipo === 'menu') {
      novo.titulo = String(b.titulo || '').trim() || null
      novo.no_nome = marcado(b.no_nome)
    } else {
      const preco = lerValor(b.preco || '')
      if (!preco) return voltar(res, `/servicos/${it.id}`, 'Preço inválido. Escreva só o número, por exemplo 50 ou 49,90.', 'erro')
      const textoPrimeira = String(b.preco_primeira || '').trim()
      const primeira = textoPrimeira ? lerValor(textoPrimeira) : null
      if (textoPrimeira && !primeira) return voltar(res, `/servicos/${it.id}`, 'Preço de primeira consulta inválido.', 'erro')
      Object.assign(novo, { preco, preco_primeira: primeira, sob_consulta: marcado(b.sob_consulta) })
    }
    banco.salvarItem(it.id, novo)
    voltar(res, `/servicos/${it.id}`, `Salvo. ${nome}: ${resumoItem(novo)}.`)
  })

  app.post('/servicos/:id/mover', (req, res) => {
    const id = idDe(req)
    const ok = banco.moverItem(id, Number(req.body.direcao) < 0 ? -1 : 1)
    const it = banco.item(id)
    const pos = it ? banco.filhos(it.pai_id).findIndex(x => x.id === id) + 1 : 0
    voltar(res, `/servicos/${id}`, ok ? `Movido. Agora é a opção ${pos}.` : `Não mudou. Já é a ${pos === 1 ? 'primeira' : 'última'} opção.`, ok ? 'ok' : 'erro')
  })

  app.get('/servicos/:id/excluir', (req, res) => {
    const it = banco.item(idDe(req))
    if (!it) return voltar(res, '/servicos', 'Item não encontrado.', 'erro')
    mostrar(req, res, `Excluir ${it.nome}?`, html`
      <p>${it.tipo === 'menu' ? 'Todas as opções dentro deste menu também serão excluídas. ' : ''}Isto não pode ser desfeito. Se for só por um tempo, prefira marcar como indisponível.</p>
      <form method="post" action="/servicos/${it.id}/excluir">
        <button type="submit">Sim, excluir ${it.nome}</button>
      </form>
      <p><a href="/servicos/${it.id}">Não, voltar</a></p>`, '/servicos')
  })

  app.post('/servicos/:id/excluir', (req, res) => {
    const it = banco.item(idDe(req))
    if (!it) return voltar(res, '/servicos', 'Item não encontrado.', 'erro')
    banco.excluirItem(it.id)
    voltar(res, it.pai_id ? `/servicos/${it.pai_id}` : '/servicos', `${it.nome} excluído.`)
  })

  // ---------- mensagens ----------
  app.get('/mensagens', (req, res) => {
    mostrar(req, res, 'Mensagens do assistente', html`
      <p>Toque numa mensagem para mudar o texto. Palavras entre chaves, como {pedido}, são trocadas pelo valor certo na hora do envio.</p>
      <ul class="lista">
        ${TEXTOS.map(t => {
          const corpo = banco.texto(t.chave)
          return html`<li><a href="/mensagens/${t.chave}"><strong>${t.rotulo}</strong></a>
            <pre class="previa">${corpo ? corpo.slice(0, 120) + (corpo.length > 120 ? '…' : '') : '(em branco: não é enviada)'}</pre></li>`
        })}
      </ul>`, '/mensagens')
  })

  app.get('/mensagens/:chave', (req, res) => {
    const t = TEXTOS_POR_CHAVE[req.params.chave]
    if (!t) return voltar(res, '/mensagens', 'Mensagem não encontrada.', 'erro')
    mostrar(req, res, t.rotulo, html`
      <form method="post" action="/mensagens/${t.chave}">
        <label for="corpo">Texto</label>
        ${t.ajuda ? html`<p class="ajuda" id="corpo-ajuda">${t.ajuda}</p>` : ''}
        <textarea id="corpo" name="corpo"${cru(t.ajuda ? ' aria-describedby="corpo-ajuda"' : '')}>${banco.texto(t.chave)}</textarea>
        <button type="submit">Salvar</button>
        <button type="submit" name="restaurar" value="1" class="secundario">Voltar ao texto original</button>
      </form>
      <p><a href="/mensagens">Voltar às mensagens</a></p>`, '/mensagens')
  })

  app.post('/mensagens/:chave', (req, res) => {
    const t = TEXTOS_POR_CHAVE[req.params.chave]
    if (!t) return voltar(res, '/mensagens', 'Mensagem não encontrada.', 'erro')
    const corpo = req.body.restaurar ? t.padrao : String(req.body.corpo || '').replace(/\r\n/g, '\n')
    banco.salvarTexto(t.chave, corpo)
    voltar(res, `/mensagens/${t.chave}`, req.body.restaurar ? 'Texto original restaurado.' : 'Salvo. Vale a partir da próxima mensagem.')
  })

  // ---------- pix ----------
  const CAMPOS_PIX = [['pix_tipo', 'Tipo de chave', 'Por exemplo: CPF, celular, e-mail ou chave aleatória.'],
    ['pix_chave', 'Chave', ''], ['pix_titular', 'Titular', ''], ['pix_banco', 'Banco', '']]
  app.get('/pix', (req, res) => {
    mostrar(req, res, 'Dados do Pix', html`
      <p>A troca vale na hora para os próximos clientes.</p>
      <form method="post" action="/pix">
        ${CAMPOS_PIX.map(([k, r, a]) => campo({ nome: k, rotulo: r, valor: banco.ajuste(k), ajuda: a || undefined, obrigatorio: true }))}
        <button type="submit">Salvar</button>
      </form>`, '/pix')
  })
  app.post('/pix', (req, res) => {
    for (const [k] of CAMPOS_PIX) {
      const v = String(req.body[k] || '').trim()
      if (!v) return voltar(res, '/pix', 'Preencha os quatro campos.', 'erro')
    }
    for (const [k] of CAMPOS_PIX) banco.salvarAjuste(k, String(req.body[k]).trim())
    voltar(res, '/pix', `Salvo. Chave ${banco.ajuste('pix_chave')}, ${banco.ajuste('pix_titular')}, ${banco.ajuste('pix_banco')}.`)
  })

  // ---------- contatos ----------
  app.get('/contatos', (req, res) => {
    const agora = Date.now()
    const lista = banco.contatos()
    mostrar(req, res, 'Contatos', html`
      <p>Contatos marcados como pessoais nunca recebem o menu do assistente. Use para família, amigos e qualquer pessoa que não seja cliente.</p>
      <form method="post" action="/contatos/novo">
        ${campo({ nome: 'telefone', rotulo: 'Adicionar contato pessoal pelo número, com DDD', tipo: 'tel', obrigatorio: true, extra: 'inputmode="tel"' })}
        <button type="submit">Adicionar como pessoal</button>
      </form>
      <h2>Quem já escreveu</h2>
      ${lista.length ? '' : html`<p>Ninguém ainda.</p>`}
      <ul class="lista">
        ${lista.map(c => {
          const tel = telefoneDoJid(c.jid) || c.jid
          const situacao = c.ignorado ? 'Pessoal: o assistente não responde.'
            : c.com_mago ? 'Com o Mago: o assistente está parado.'
              : c.pausado_ate > agora ? `Pausado até ${dataHoraCurta(c.pausado_ate)}, porque o Mago escreveu.`
                : 'O assistente atende.'
          return html`<li><strong>${c.nome_whatsapp || 'Sem nome'}</strong>, ${tel}. ${c.pedidos} ${c.pedidos === 1 ? 'pedido' : 'pedidos'}.<br>${situacao}
            <form method="post" action="/contatos/alterar">
              <input type="hidden" name="jid" value="${c.jid}">
              ${c.ignorado
                ? html`<button name="acao" value="atender" class="secundario">Não é pessoal: atender</button>`
                : html`<button name="acao" value="ignorar" class="secundario">Marcar como pessoal</button>`}
              ${!c.ignorado && (c.com_mago || c.pausado_ate > agora) ? html`<button name="acao" value="devolver" class="secundario">Devolver ao assistente agora</button>` : ''}
            </form></li>`
        })}
      </ul>`, '/contatos')
  })
  app.post('/contatos/novo', (req, res) => {
    const tel = normalizarTelefone(req.body.telefone)
    if (tel.length < 12) return voltar(res, '/contatos', 'Número inválido. Escreva com DDD, por exemplo 51 99999 0000.', 'erro')
    const jid = `${tel}@s.whatsapp.net`
    banco.tocarCliente(jid, null, Date.now())
    banco.marcarIgnorado(jid, true)
    voltar(res, '/contatos', `${tel} marcado como pessoal.`)
  })
  app.post('/contatos/alterar', (req, res) => {
    const c = banco.cliente(String(req.body.jid || ''))
    if (!c) return voltar(res, '/contatos', 'Contato não encontrado.', 'erro')
    const nome = c.nome_whatsapp || telefoneDoJid(c.jid)
    if (req.body.acao === 'ignorar') banco.marcarIgnorado(c.jid, true)
    else if (req.body.acao === 'atender') banco.marcarIgnorado(c.jid, false)
    else if (req.body.acao === 'devolver') { banco.marcarComMago(c.jid, false); banco.salvarEstado(c.jid, null) }
    const texto = { ignorar: `${nome} marcado como pessoal.`, atender: `${nome}: o assistente volta a atender.`, devolver: `${nome} devolvido ao assistente.` }[req.body.acao] || 'Nada mudou.'
    voltar(res, '/contatos', texto)
  })

  // ---------- ajustes ----------
  const CAMPOS_AJUSTES = [
    ['pausa_horas', 'Horas de silêncio quando o Mago escreve numa conversa', 'Se o Mago responder um cliente pelo celular, o assistente fica quieto nessa conversa por este tempo. Zero desliga.', true],
    ['limite_horas', 'Horas até uma conversa parada recomeçar', 'Se o cliente parar no meio (antes do comprovante) e voltar depois deste tempo, recomeça do menu.', true],
    ['retencao_dias', 'Dias até apagar fotos e comprovantes', 'O número aparece na mensagem que pede a foto.', true],
    ['aviso_numero', 'Número que recebe os pedidos (opcional)', 'Em branco, os pedidos chegam na conversa do Mago com ele mesmo. Se preencher, chegam neste número, e os comandos também são aceitos dele.', false]
  ]
  const CAMPOS_COMANDOS = [['cmd_pagou', 'Confirmar pagamento'], ['cmd_assumir', 'Assumir cliente'], ['cmd_liberar', 'Liberar cliente'],
    ['cmd_valor', 'Enviar valor do trabalho'], ['cmd_hoje', 'Pedidos de hoje'], ['cmd_ajuda', 'Lista de comandos']]
  app.get('/ajustes', (req, res) => {
    mostrar(req, res, 'Ajustes', html`
      <form method="post" action="/ajustes">
        ${CAMPOS_AJUSTES.map(([k, r, a, num]) => campo({ nome: k, rotulo: r, valor: banco.ajuste(k), ajuda: a, tipo: num ? 'number' : 'tel', extra: num ? 'min="0" inputmode="numeric"' : 'inputmode="tel"' }))}
        <h2>Palavras dos comandos</h2>
        <p class="ajuda">O que o Mago escreve ou dita antes do número do pedido.</p>
        ${CAMPOS_COMANDOS.map(([k, r]) => campo({ nome: k, rotulo: r, valor: banco.ajuste(k), ajuda: `Original: ${AJUSTES_PADRAO[k]}`, obrigatorio: true }))}
        <button type="submit">Salvar</button>
      </form>`, '/ajustes')
  })
  app.post('/ajustes', (req, res) => {
    for (const [k, r, , num] of CAMPOS_AJUSTES) {
      const v = String(req.body[k] ?? '').trim()
      if (num && !/^\d+$/.test(v)) return voltar(res, '/ajustes', `${r}: escreva só números.`, 'erro')
    }
    const palavras = CAMPOS_COMANDOS.map(([k]) => String(req.body[k] || '').trim().toLowerCase())
    if (palavras.some(p => !p)) return voltar(res, '/ajustes', 'Nenhuma palavra de comando pode ficar vazia.', 'erro')
    if (new Set(palavras).size !== palavras.length) return voltar(res, '/ajustes', 'Dois comandos estão com a mesma palavra.', 'erro')
    for (const [k, , , num] of CAMPOS_AJUSTES) {
      const v = String(req.body[k] ?? '').trim()
      banco.salvarAjuste(k, num ? v : (v ? normalizarTelefone(v) : ''))
    }
    CAMPOS_COMANDOS.forEach(([k], i) => banco.salvarAjuste(k, palavras[i]))
    voltar(res, '/ajustes', 'Salvo.')
  })

  // ---------- whatsapp ----------
  app.get('/whatsapp', async (req, res) => {
    const e = whatsapp.estado
    const qr = await whatsapp.qrImagem()
    const textos = {
      conectado: `Conectado ao número ${e.numero}.`,
      aguardando_pareamento: 'Esperando conexão. Use uma das formas abaixo.',
      reconectando: 'A conexão caiu e o assistente está tentando de novo. Atualize em alguns segundos.',
      desconectado: 'Desconectado. Atualize em alguns segundos para conectar de novo.',
      iniciando: 'Iniciando. Atualize em alguns segundos.'
    }
    mostrar(req, res, 'Conexão com o WhatsApp', html`
      <p>${textos[e.situacao]}</p>
      <form method="get" action="/whatsapp"><button type="submit" class="secundario">Atualizar</button></form>
      ${e.situacao === 'aguardando_pareamento' ? html`
        <h2>Conectar com código de 8 letras</h2>
        <p>No celular do Mago, abra o WhatsApp, toque em Mais opções, depois Dispositivos conectados, Conectar dispositivo, e então "Conectar com número de telefone". Digite o código que aparecer aqui.</p>
        <form method="post" action="/whatsapp/codigo">
          ${campo({ nome: 'telefone', rotulo: 'Número do WhatsApp do Mago, com DDD', tipo: 'tel', obrigatorio: true, extra: 'inputmode="tel"' })}
          <button type="submit">Gerar código</button>
        </form>
        ${qr ? html`<h2>Ou ler o QR</h2><img src="${qr}" alt="Código QR para conectar o WhatsApp. Leia pela câmera em Dispositivos conectados." width="320" height="320">` : ''}` : ''}`, '/whatsapp')
  })
  app.post('/whatsapp/codigo', async (req, res) => {
    try {
      const codigo = await whatsapp.pedirCodigoPareamento(normalizarTelefone(req.body.telefone))
      const soletrado = codigo.split('').join(' ')
      mostrar(req, res, 'Código de conexão', html`
        <p id="codigo" style="font-size:2rem;font-weight:700;letter-spacing:.2em">${codigo}</p>
        <p>Letra por letra: ${soletrado}.</p>
        <p>Digite no celular do Mago, em Dispositivos conectados, Conectar com número de telefone. O código vale por poucos minutos.</p>
        <p><a href="/whatsapp">Ver se conectou</a></p>`, '/whatsapp')
    } catch (e) {
      log.error({ err: e }, 'falha ao pedir código de pareamento')
      voltar(res, '/whatsapp', `Não consegui gerar o código: ${e.message}`, 'erro')
    }
  })

  // ---------- usuários ----------
  app.get('/usuarios', (req, res) => {
    mostrar(req, res, 'Usuários do painel', html`
      <ul class="lista">
        ${banco.usuarios().map(u => html`<li><strong>${u.nome}</strong>. WhatsApp: ${u.telefone || 'nenhum'}. Usuário: ${u.usuario || 'nenhum'}. ${u.senha ? 'Tem senha.' : 'Sem senha.'}
          ${u.id !== req.usuario.id ? html`<form method="post" action="/usuarios/${u.id}/excluir"><button type="submit" class="secundario">Remover ${u.nome}</button></form>` : ''}</li>`)}
      </ul>
      <h2>Adicionar usuário</h2>
      <form method="post" action="/usuarios">
        ${campo({ nome: 'nome', rotulo: 'Nome', obrigatorio: true })}
        ${campo({ nome: 'telefone', rotulo: 'Número de WhatsApp, com DDD', tipo: 'tel', ajuda: 'Para entrar por link ou código.', extra: 'inputmode="tel"' })}
        ${campo({ nome: 'usuario', rotulo: 'Usuário (opcional)', extra: 'autocapitalize="none"' })}
        ${campo({ nome: 'senha', rotulo: 'Senha (opcional, mínimo 8 caracteres)', tipo: 'password', extra: 'autocomplete="new-password"' })}
        <button type="submit">Adicionar</button>
      </form>`, '/usuarios')
  })
  function validarUsuario (b, id) {
    const u = {
      nome: String(b.nome || '').trim(),
      telefone: b.telefone ? normalizarTelefone(b.telefone) : '',
      usuario: String(b.usuario || '').trim().toLowerCase()
    }
    if (!u.nome) return { erro: 'Escreva o nome.' }
    if (u.telefone && u.telefone.length < 12) return { erro: 'Número inválido. Escreva com DDD.' }
    if (!u.telefone && !u.usuario) return { erro: 'Informe o WhatsApp ou um usuário, senão a pessoa não consegue entrar.' }
    const t = u.telefone && banco.usuarioPorTelefone(u.telefone)
    if (t && t.id !== id) return { erro: 'Esse número já é de outro usuário.' }
    const l = u.usuario && banco.usuarioPorLogin(u.usuario)
    if (l && l.id !== id) return { erro: 'Esse usuário já existe.' }
    return { u }
  }
  app.post('/usuarios', (req, res) => {
    const { u, erro } = validarUsuario(req.body)
    if (erro) return voltar(res, '/usuarios', erro, 'erro')
    const senha = String(req.body.senha || '')
    if (senha && senha.length < 8) return voltar(res, '/usuarios', 'A senha precisa de pelo menos 8 caracteres.', 'erro')
    if (u.usuario && !senha) return voltar(res, '/usuarios', 'Quem tem usuário precisa de senha.', 'erro')
    banco.criarUsuario({ ...u, senha: senha ? hashSenha(senha) : null })
    voltar(res, '/usuarios', `${u.nome} adicionado.`)
  })
  app.post('/usuarios/:id/excluir', (req, res) => {
    const u = banco.usuario(idDe(req))
    if (!u || u.id === req.usuario.id) return voltar(res, '/usuarios', 'Não é possível remover.', 'erro')
    banco.excluirUsuario(u.id)
    voltar(res, '/usuarios', `${u.nome} removido.`)
  })

  app.get('/conta', (req, res) => {
    const u = req.usuario
    mostrar(req, res, 'Minha conta', html`
      <form method="post" action="/conta">
        ${campo({ nome: 'nome', rotulo: 'Nome', valor: u.nome, obrigatorio: true })}
        ${campo({ nome: 'telefone', rotulo: 'Número de WhatsApp, com DDD', valor: u.telefone || '', tipo: 'tel', extra: 'inputmode="tel"' })}
        ${campo({ nome: 'usuario', rotulo: 'Usuário', valor: u.usuario || '', extra: 'autocapitalize="none"' })}
        ${campo({ nome: 'senha', rotulo: 'Nova senha', tipo: 'password', ajuda: 'Deixe em branco para manter a atual. Mínimo 8 caracteres.', extra: 'autocomplete="new-password"' })}
        <button type="submit">Salvar</button>
      </form>`, '/conta')
  })
  app.post('/conta', (req, res) => {
    const { u, erro } = validarUsuario(req.body, req.usuario.id)
    if (erro) return voltar(res, '/conta', erro, 'erro')
    const senha = String(req.body.senha || '')
    if (senha && senha.length < 8) return voltar(res, '/conta', 'A senha precisa de pelo menos 8 caracteres.', 'erro')
    if (u.usuario && !senha && !req.usuario.senha) return voltar(res, '/conta', 'Para ter usuário, defina uma senha.', 'erro')
    banco.salvarUsuario(req.usuario.id, u)
    if (senha) banco.salvarSenha(req.usuario.id, hashSenha(senha))
    voltar(res, '/conta', 'Salvo.')
  })

  app.use((req, res) => res.status(404).send(pagina({ titulo: 'Página não encontrada', usuario: req.usuario, corpo: '<p><a href="/">Voltar ao início</a></p>' })))
  app.use((err, req, res, next) => {
    log.error({ err }, 'erro no painel')
    res.status(500).send(pagina({ titulo: 'Algo deu errado', usuario: req.usuario, corpo: '<p>Tente de novo. Se continuar, avise o Ricardo.</p><p><a href="/">Voltar ao início</a></p>' }))
  })

  return app
}
