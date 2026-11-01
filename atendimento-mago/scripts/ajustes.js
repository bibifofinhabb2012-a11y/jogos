// Muda ajustes pela linha de comando (usado pelo instalador).
//   npm run ajustes -- pix_tipo=CPF "pix_titular=Nome Sobrenome"
import fs from 'node:fs'
import path from 'node:path'
import { abrirBanco, AJUSTES_PADRAO } from '../src/db.js'

const pares = process.argv.slice(2).map(a => {
  const i = a.indexOf('=')
  return i > 0 ? [a.slice(0, i), a.slice(i + 1).trim()] : [a, null]
})
const invalidos = pares.filter(([k, v]) => !(k in AJUSTES_PADRAO) || v == null)
if (!pares.length || invalidos.length) {
  console.error('Uso: npm run ajustes -- chave=valor ...')
  console.error('Chaves aceitas: ' + Object.keys(AJUSTES_PADRAO).join(', '))
  process.exit(1)
}
const pastaDados = path.resolve(process.env.PASTA_DADOS || 'dados')
fs.mkdirSync(pastaDados, { recursive: true })
const banco = abrirBanco(path.join(pastaDados, 'mago.db'))
for (const [k, v] of pares) banco.salvarAjuste(k, v)
console.log('Ajustes salvos: ' + pares.map(([k]) => k).join(', '))
