// Entrada no painel: link pelo WhatsApp, código pelo WhatsApp, ou usuário e senha.
import crypto from 'node:crypto'
import { normalizarTelefone } from '../util.js'
import { pagina, html, campo } from './html.js'

const MINUTO = 60 * 1000
const DEZ_MIN = 10 * MINUTO
const TRINTA_DIAS = 30 * 24 * 60 * MINUTO
const COOKIE = 'sessao'

export const sha = (s) => crypto.createHash('sha256').update(s).digest('hex')

// Cria um link de entrada de uso único, válido por 10 minutos.
export function criarLinkAcesso (banco, usuarioId, enderecoPublico) {
  const token = crypto.randomBytes(24).toString('base64url')
  banco.apagarCodigos(usuarioId)
  banco.criarAcesso(sha(token), usuarioId, 'link', Date.now() + DEZ_MIN)
  return `${enderecoPublico}/entrar/link/${token}`
}

export function hashSenha (senha) {
  const sal = crypto.randomBytes(16).toString('hex')
  return `scrypt$${sal}$${crypto.scryptSync(senha, sal, 64).toString('hex')}`
}

export function conferirSenha (senha, guardada) {
  if (!guardada) return false
  const [, sal, hash] = guardada.split('$')
  const calc = crypto.scryptSync(senha, sal, 64)
  const esperado = Buffer.from(hash, 'hex')
  return esperado.length === calc.length && crypto.timingSafeEqual(calc, esperado)
}

function lerCookies (req) {
  const out = {}
  for (const par of (req.headers.cookie || '').split(';')) {
    const i = par.indexOf('=')
    if (i > 0) out[par.slice(0, i).trim()] = decodeURIComponent(par.slice(i + 1).trim())
  }
  return out
}

// Limite simples de tentativas por chave (IP ou telefone), em memória.
function criarLimite (max, janela) {
  const mapa = new Map()
  return (chave) => {
    const agora = Date.now()
    const lista = (mapa.get(chave) || []).filter(t => agora - t < janela)
    lista.push(agora)
    mapa.set(chave, lista)
    return lista.length <= max
  }
}

export function montarAcesso ({ app, banco, whatsapp, enderecoPublico }) {
  const seguro = enderecoPublico.startsWith('https://')
  const limiteIp = criarLimite(20, 15 * MINUTO)
  const limiteTelefone = criarLimite(5, 15 * MINUTO)

  function abrirSessao (res, usuarioId) {
    const token = crypto.randomBytes(32).toString('base64url')
    banco.criarAcesso(sha(token), usuarioId, 'sessao', Date.now() + TRINTA_DIAS)
    res.setHeader('Set-Cookie', `${COOKIE}=${token}; Path=/; HttpOnly; SameSite=Lax; Max-Age=${TRINTA_DIAS / 1000}${seguro ? '; Secure' : ''}`)
  }

  // Descobre quem está logado. Rotas do painel exigem req.usuario.
  app.use((req, res, next) => {
    const token = lerCookies(req)[COOKIE]
    if (token) {
      const a = banco.acesso(sha(token))
      if (a && a.tipo === 'sessao' && a.expira > Date.now()) req.usuario = banco.usuario(a.usuario_id)
    }
    next()
  })

  function telaEntrar (res, aviso, status = 200) {
    res.status(status).send(pagina({
      titulo: 'Entrar no painel',
      aviso,
      corpo: html`
        <h2>Pelo WhatsApp</h2>
        <form method="post" action="/entrar/whatsapp">
          ${campo({ nome: 'telefone', rotulo: 'Seu número de WhatsApp, com DDD', tipo: 'tel', obrigatorio: true, extra: 'autocomplete="tel" inputmode="tel"' })}
          <button type="submit" name="forma" value="link">Receber link de acesso</button>
          <button type="submit" name="forma" value="codigo" class="secundario">Receber código</button>
        </form>
        <h2>Com usuário e senha</h2>
        <form method="post" action="/entrar/senha">
          ${campo({ nome: 'usuario', rotulo: 'Usuário', obrigatorio: true, extra: 'autocomplete="username" autocapitalize="none"' })}
          ${campo({ nome: 'senha', rotulo: 'Senha', tipo: 'password', obrigatorio: true, extra: 'autocomplete="current-password"' })}
          <button type="submit">Entrar</button>
        </form>`
    }))
  }

  function telaCodigo (res, telefone, aviso) {
    res.send(pagina({
      titulo: 'Digite o código',
      aviso,
      corpo: html`
        <form method="post" action="/entrar/codigo">
          <input type="hidden" name="telefone" value="${telefone}">
          ${campo({ nome: 'codigo', rotulo: 'Código de 6 números', ajuda: 'O código vale por 10 minutos.', obrigatorio: true, extra: 'inputmode="numeric" autocomplete="one-time-code" autofocus' })}
          <button type="submit">Entrar</button>
        </form>
        <p><a href="/entrar">Voltar</a></p>`
    }))
  }

  app.get('/entrar', (req, res) => {
    if (req.usuario) return res.redirect('/')
    telaEntrar(res)
  })

  app.post('/entrar/whatsapp', async (req, res) => {
    const telefone = normalizarTelefone(req.body.telefone)
    const forma = req.body.forma === 'codigo' ? 'codigo' : 'link'
    if (!limiteIp(req.ip) || !limiteTelefone(telefone)) {
      return telaEntrar(res, { tipo: 'erro', texto: 'Muitas tentativas. Espere 15 minutos.' }, 429)
    }
    if (whatsapp.estado.situacao !== 'conectado') {
      return telaEntrar(res, { tipo: 'erro', texto: 'O WhatsApp do assistente está desconectado agora. Entre com usuário e senha.' })
    }
    const u = banco.usuarioPorTelefone(telefone)
    if (u) {
      try {
        const jid = await whatsapp.jidDoTelefone(telefone)
        if (jid) {
          banco.apagarCodigos(u.id)
          if (forma === 'link') {
            const link = criarLinkAcesso(banco, u.id, enderecoPublico)
            await whatsapp.envio.enviarTexto(jid, `Link de acesso ao painel do Mago. Vale por 10 minutos:\n${link}`)
          } else {
            const codigo = String(crypto.randomInt(0, 1000000)).padStart(6, '0')
            banco.criarAcesso(sha(`${u.id}:${codigo}`), u.id, 'codigo', Date.now() + DEZ_MIN)
            await whatsapp.envio.enviarTexto(jid, `${codigo} é o seu código de acesso ao painel do Mago. Vale por 10 minutos.`)
          }
        }
      } catch (e) {
        req.log?.error({ err: e }, 'falha ao enviar acesso')
      }
    }
    const texto = forma === 'link'
      ? 'Se este número estiver cadastrado, o link chega no WhatsApp em instantes. Toque nele para entrar.'
      : 'Se este número estiver cadastrado, o código chega no WhatsApp em instantes.'
    if (forma === 'link') return telaEntrar(res, { tipo: 'ok', texto })
    telaCodigo(res, telefone, { tipo: 'ok', texto })
  })

  app.post('/entrar/codigo', (req, res) => {
    const telefone = normalizarTelefone(req.body.telefone)
    const codigo = String(req.body.codigo || '').replace(/\D/g, '')
    if (!limiteIp(req.ip)) return telaEntrar(res, { tipo: 'erro', texto: 'Muitas tentativas. Espere 15 minutos.' }, 429)
    const u = banco.usuarioPorTelefone(telefone)
    const a = u && banco.codigoAtivo(u.id, Date.now())
    if (!a) return telaEntrar(res, { tipo: 'erro', texto: 'O código venceu ou não existe. Peça outro.' })
    if (a.tentativas >= 5) {
      banco.apagarAcesso(a.hash)
      return telaEntrar(res, { tipo: 'erro', texto: 'Código errado muitas vezes. Peça outro.' })
    }
    if (sha(`${u.id}:${codigo}`) !== a.hash) {
      banco.somarTentativa(a.hash)
      return telaCodigo(res, telefone, { tipo: 'erro', texto: 'Código errado. Confira e digite de novo.' })
    }
    banco.apagarAcesso(a.hash)
    abrirSessao(res, u.id)
    res.redirect('/')
  })

  app.get('/entrar/link/:token', (req, res) => {
    const a = banco.acesso(sha(req.params.token))
    if (!a || a.tipo !== 'link' || a.expira < Date.now()) {
      return telaEntrar(res, { tipo: 'erro', texto: 'Este link venceu ou já foi usado. Peça outro.' }, 410)
    }
    banco.apagarAcesso(a.hash)
    abrirSessao(res, a.usuario_id)
    res.redirect('/')
  })

  app.post('/entrar/senha', (req, res) => {
    if (!limiteIp(req.ip)) return telaEntrar(res, { tipo: 'erro', texto: 'Muitas tentativas. Espere 15 minutos.' }, 429)
    const u = banco.usuarioPorLogin(String(req.body.usuario || '').trim().toLowerCase())
    if (!u || !conferirSenha(String(req.body.senha || ''), u.senha)) {
      return telaEntrar(res, { tipo: 'erro', texto: 'Usuário ou senha errados.' }, 401)
    }
    abrirSessao(res, u.id)
    res.redirect('/')
  })

  app.post('/sair', (req, res) => {
    const token = lerCookies(req)[COOKIE]
    if (token) banco.apagarAcesso(sha(token))
    res.setHeader('Set-Cookie', `${COOKIE}=; Path=/; HttpOnly; SameSite=Lax; Max-Age=0`)
    res.redirect('/entrar')
  })

  // Daqui para baixo, só quem entrou.
  app.use((req, res, next) => {
    if (!req.usuario) return res.redirect('/entrar')
    next()
  })
}
