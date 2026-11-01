// Funções pequenas usadas em várias partes: texto, dinheiro e datas.

export const TIMEZONE = 'America/Sao_Paulo'

const MESES = ['janeiro', 'fevereiro', 'março', 'abril', 'maio', 'junho', 'julho',
  'agosto', 'setembro', 'outubro', 'novembro', 'dezembro']
const MESES_CURTOS = ['jan', 'fev', 'mar', 'abr', 'mai', 'jun', 'jul', 'ago', 'set', 'out', 'nov', 'dez']

export function semAcento (s) {
  return String(s).normalize('NFD').replace(/[̀-ͯ]/g, '')
}

// Troca {chave} pelo valor. Chaves desconhecidas ficam como estão, para o erro aparecer no teste.
export function preencher (modelo, vars) {
  return String(modelo).replace(/\{(\w+)\}/g, (m, k) => (k in vars && vars[k] != null ? String(vars[k]) : m))
}

// Para o cliente: "R$ 130" ou "R$ 130,50".
export function brl (centavos) {
  const reais = Math.floor(centavos / 100)
  const cent = centavos % 100
  const milhar = reais.toLocaleString('pt-BR')
  return cent ? `R$ ${milhar},${String(cent).padStart(2, '0')}` : `R$ ${milhar}`
}

// Para o Mago ouvir no TalkBack: "130 reais", "130 reais e 50 centavos", "1 real".
export function reaisFalado (centavos) {
  const reais = Math.floor(centavos / 100)
  const cent = centavos % 100
  let s = reais === 1 ? '1 real' : `${reais} reais`
  if (cent) s += ` e ${cent} ${cent === 1 ? 'centavo' : 'centavos'}`
  if (reais === 0 && cent) s = `${cent} centavos`
  return s
}

// "130", "130,50", "R$ 1.300,00", "200 reais" -> centavos. Retorna null se não entender.
export function lerValor (texto) {
  const s = String(texto).replace(/r\$|reais|real/gi, '').trim()
  const m = s.match(/^(\d{1,3}(?:\.\d{3})+|\d+)(?:,(\d{1,2}))?$/)
  if (!m) return null
  const reais = parseInt(m[1].replace(/\./g, ''), 10)
  const cent = m[2] ? parseInt(m[2].padEnd(2, '0'), 10) : 0
  const total = reais * 100 + cent
  return total > 0 ? total : null
}

function partesData (ts) {
  const p = new Intl.DateTimeFormat('en-CA', {
    timeZone: TIMEZONE, year: 'numeric', month: '2-digit', day: '2-digit', hour: '2-digit', minute: '2-digit', hourCycle: 'h23'
  }).formatToParts(new Date(ts))
  const o = {}
  for (const x of p) o[x.type] = x.value
  return o
}

// "2026-10-31" no fuso de São Paulo, para saber se um pedido é "de hoje".
export function diaLocal (ts) {
  const o = partesData(ts)
  return `${o.year}-${o.month}-${o.day}`
}

export function dataHoraCurta (ts) {
  const o = partesData(ts)
  return `${o.day}/${o.month}/${o.year} às ${o.hour}:${o.minute}`
}

// Data de nascimento escrita pelo cliente -> "1990-05-03", ou null.
// Aceita 03/05/1990, 3-5-90, 03051990, "3 de maio de 1990", "3 mai 1990".
export function lerNascimento (texto, agora = Date.now()) {
  const s = semAcento(String(texto).toLowerCase()).replace(/[º°ª]/g, '').replace(/\s+/g, ' ').trim()
  let d, m, a, r
  if ((r = s.match(/^(\d{1,2}) ?[/\-. ] ?(\d{1,2}) ?[/\-. ] ?(\d{2}|\d{4})$/))) {
    [d, m, a] = [r[1], r[2], r[3]].map(Number)
    if (r[3].length === 2) a = doisDigitos(a, agora)
  } else if ((r = s.match(/^(\d{2})(\d{2})(\d{4})$/))) {
    [d, m, a] = [r[1], r[2], r[3]].map(Number)
  } else if ((r = s.match(/^(\d{1,2}) (?:de )?([a-z]+)\.? (?:de )?(\d{2}|\d{4})$/))) {
    d = Number(r[1])
    m = MESES_CURTOS.indexOf(r[2].slice(0, 3)) + 1
    a = Number(r[3])
    if (r[3].length === 2) a = doisDigitos(a, agora)
    if (!m) return null
  } else {
    return null
  }
  const dt = new Date(Date.UTC(a, m - 1, d))
  if (dt.getUTCFullYear() !== a || dt.getUTCMonth() !== m - 1 || dt.getUTCDate() !== d) return null
  if (a < 1900 || dt.getTime() > agora) return null
  return `${a}-${String(m).padStart(2, '0')}-${String(d).padStart(2, '0')}`
}

function doisDigitos (a, agora) {
  const atual = Number(diaLocal(agora).slice(2, 4))
  return a <= atual ? 2000 + a : 1900 + a
}

export function nascimentoExtenso (iso) {
  const [a, m, d] = iso.split('-').map(Number)
  return `${d} de ${MESES[m - 1]} de ${a}`
}

export function nascimentoCurto (iso) {
  const [a, m, d] = iso.split('-')
  return `${d}/${m}/${a}`
}

// Normaliza o que o Mago escreve ou dita: minúsculas, sem acento, sem pontuação (mantém vírgula entre números).
export function normalizarComando (texto) {
  return semAcento(String(texto).toLowerCase())
    .replace(/(\d)\.(?=\d{3}\b)/g, '$1')
    .replace(/(\d),(\d)/g, '$1§$2')
    .replace(/r\$/g, ' ')
    .replace(/[^a-z0-9§ ]+/g, ' ')
    .replace(/§/g, ',')
    .replace(/\s+/g, ' ')
    .trim()
}

export function soDigitos (s) {
  return String(s || '').replace(/\D/g, '')
}

// Número digitado no painel -> só dígitos com DDI. "51 99999-0000" -> "5551999990000".
export function normalizarTelefone (s) {
  let d = soDigitos(s)
  if (d.length === 10 || d.length === 11) d = '55' + d
  return d
}

// Números brasileiros de celular aparecem com e sem o 9 depois do DDD. Retorna as duas formas.
export function variantesTelefone (d) {
  d = soDigitos(d)
  const v = [d]
  if (d.startsWith('55') && d.length === 13 && d[4] === '9') v.push(d.slice(0, 4) + d.slice(5))
  if (d.startsWith('55') && d.length === 12) v.push(d.slice(0, 4) + '9' + d.slice(4))
  return v
}

export function telefoneDoJid (jid) {
  if (!jid || !jid.endsWith('@s.whatsapp.net')) return ''
  return jid.split('@')[0].split(':')[0]
}
