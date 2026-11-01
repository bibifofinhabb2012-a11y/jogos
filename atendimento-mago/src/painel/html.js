// Pedaços de HTML do painel. Tudo gerado no servidor, sem JavaScript de tela,
// para o TalkBack ler a página como ela é.

export function esc (s) {
  return String(s ?? '').replace(/[&<>"']/g, c => ({ '&': '&amp;', '<': '&lt;', '>': '&gt;', '"': '&quot;', "'": '&#39;' }[c]))
}

// html`...` escapa as interpolações; use cru() para HTML já montado.
class Cru { constructor (s) { this.s = s } toString () { return this.s } }
export const cru = (s) => new Cru(s)
export function html (partes, ...vals) {
  let out = partes[0]
  vals.forEach((v, i) => {
    const s = Array.isArray(v) ? v.map(x => (x instanceof Cru ? x.s : esc(x))).join('') : (v instanceof Cru ? v.s : esc(v))
    out += s + partes[i + 1]
  })
  return cru(out)
}

const CSS = `
:root { --fundo:#ffffff; --texto:#111111; --suave:#4a4a4a; --borda:#767676; --destaque:#0b4fa8; --destaque-texto:#ffffff;
  --ok-fundo:#e6f4ea; --erro-fundo:#fdecea; --campo:#ffffff; }
@media (prefers-color-scheme: dark) { :root { --fundo:#121212; --texto:#f2f2f2; --suave:#c4c4c4; --borda:#9a9a9a;
  --destaque:#8ab4f8; --destaque-texto:#0b0b0b; --ok-fundo:#16351f; --erro-fundo:#45201c; --campo:#1e1e1e; } }
* { box-sizing: border-box; }
body { margin:0; background:var(--fundo); color:var(--texto); font: 18px/1.5 system-ui, -apple-system, "Segoe UI", Roboto, sans-serif; }
.pular { position:absolute; left:-9999px; } .pular:focus { left:16px; top:8px; background:var(--fundo); padding:8px; z-index:2; }
header { border-bottom:2px solid var(--borda); padding:8px 16px; }
nav ul { list-style:none; margin:0; padding:0; display:flex; flex-wrap:wrap; gap:4px 16px; }
nav a { display:inline-block; padding:10px 0; }
nav form { display:inline; }
main { max-width:760px; margin:0 auto; padding:16px; }
a { color:var(--destaque); }
h1 { font-size:1.6rem; margin:8px 0 16px; } h2 { font-size:1.3rem; margin:24px 0 8px; }
label { display:block; font-weight:600; margin:16px 0 4px; }
.ajuda { color:var(--suave); margin:2px 0 6px; font-size:.95rem; }
input[type=text], input[type=tel], input[type=password], input[type=number], textarea, select {
  width:100%; font:inherit; padding:12px; border:2px solid var(--borda); border-radius:6px; background:var(--campo); color:var(--texto); }
textarea { min-height:12em; }
.marcar { display:flex; gap:12px; align-items:center; font-weight:600; margin:16px 0; }
.marcar input { width:28px; height:28px; flex:none; }
fieldset { border:2px solid var(--borda); border-radius:6px; margin:16px 0; padding:8px 16px 16px; }
legend { font-weight:600; padding:0 4px; }
button, .botao { display:inline-block; min-height:48px; font:inherit; font-weight:600; padding:10px 18px; margin:12px 8px 0 0;
  border:2px solid var(--destaque); border-radius:6px; background:var(--destaque); color:var(--destaque-texto); cursor:pointer; text-decoration:none; }
button.secundario { background:transparent; color:var(--destaque); }
:focus-visible { outline:3px solid var(--destaque); outline-offset:2px; }
.aviso { padding:12px 16px; border:2px solid var(--borda); border-radius:6px; margin:0 0 16px; }
.aviso.ok { background:var(--ok-fundo); } .aviso.erro { background:var(--erro-fundo); }
ul.lista { list-style:none; padding:0; margin:0; } ul.lista > li { border-bottom:1px solid var(--borda); padding:12px 0; }
ul.arvore ul { padding-left:20px; border-left:2px solid var(--borda); }
ul.arvore li { margin:8px 0; }
dl { margin:0; } dt { font-weight:600; margin-top:12px; } dd { margin:0; }
img.anexo { max-width:100%; height:auto; border:1px solid var(--borda); margin-top:8px; }
pre.previa { white-space:pre-wrap; font:inherit; color:var(--suave); margin:4px 0 0; }
`

const MENU = [
  ['/', 'Início'], ['/pedidos', 'Pedidos'], ['/servicos', 'Serviços e preços'], ['/mensagens', 'Mensagens'],
  ['/pix', 'Pix'], ['/contatos', 'Contatos'], ['/ajustes', 'Ajustes'], ['/whatsapp', 'WhatsApp'],
  ['/usuarios', 'Usuários'], ['/conta', 'Minha conta']
]

/**
 * aviso: { texto, tipo: 'ok'|'erro' } — lido em voz alta ao abrir a página.
 */
export function pagina ({ titulo, corpo, usuario, aviso, caminho = '' }) {
  const prefixo = aviso ? (aviso.tipo === 'erro' ? 'Erro. ' : '') + aviso.texto.split('.')[0] + '. ' : ''
  const nav = usuario
    ? html`<header><nav aria-label="Menu do painel"><ul>
        ${MENU.map(([href, nome]) => html`<li><a href="${href}"${cru(caminho === href ? ' aria-current="page"' : '')}>${nome}</a></li>`)}
        <li><form method="post" action="/sair"><button type="submit" class="secundario" style="margin:0">Sair</button></form></li>
      </ul></nav></header>`
    : ''
  const caixa = aviso
    ? html`<p id="aviso" class="aviso ${aviso.tipo}" role="${aviso.tipo === 'erro' ? 'alert' : 'status'}" tabindex="-1">${aviso.texto}</p>`
    : ''
  return `<!doctype html>
<html lang="pt-BR">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>${esc(prefixo + titulo)} · Painel do Mago</title>
<style>${CSS}</style>
</head>
<body>
<a class="pular" href="#conteudo">Pular para o conteúdo</a>
${nav}
<main id="conteudo">
<h1>${esc(titulo)}</h1>
${caixa}
${corpo}
</main>
${aviso ? '<script>document.getElementById("aviso").focus()</script>' : ''}
</body>
</html>`
}

export function campo ({ nome, rotulo, valor = '', tipo = 'text', ajuda, obrigatorio, extra = '' }) {
  const idAjuda = ajuda ? `${nome}-ajuda` : ''
  return html`<label for="${nome}">${rotulo}</label>
    ${ajuda ? html`<p class="ajuda" id="${idAjuda}">${ajuda}</p>` : ''}
    <input type="${tipo}" id="${nome}" name="${nome}" value="${valor}"${cru(obrigatorio ? ' required' : '')}${cru(idAjuda ? ` aria-describedby="${idAjuda}"` : '')}${cru(extra ? ' ' + extra : '')}>`
}

export function caixaMarcar ({ nome, rotulo, marcado, ajuda }) {
  const idAjuda = ajuda ? `${nome}-ajuda` : ''
  return html`<div class="marcar"><input type="checkbox" id="${nome}" name="${nome}"${cru(marcado ? ' checked' : '')}${cru(idAjuda ? ` aria-describedby="${idAjuda}"` : '')}>
    <label for="${nome}" style="margin:0">${rotulo}</label></div>
    ${ajuda ? html`<p class="ajuda" id="${idAjuda}">${ajuda}</p>` : ''}`
}
