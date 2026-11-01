// Cria ou atualiza um usuário do painel pela linha de comando. Necessário para o primeiro acesso.
//   npm run usuario -- --nome Ricardo --usuario ricardo --senha "uma senha boa" --numero 51999990000
//   npm run usuario -- --nome Mago --numero 51988880000
import fs from 'node:fs'
import path from 'node:path'
import { parseArgs } from 'node:util'
import { abrirBanco } from '../src/db.js'
import { hashSenha } from '../src/painel/acesso.js'
import { normalizarTelefone } from '../src/util.js'

const { values: a } = parseArgs({
  options: { nome: { type: 'string' }, usuario: { type: 'string' }, senha: { type: 'string' }, numero: { type: 'string' } }
})
// A senha também pode vir pela variável SENHA_PAINEL, para não aparecer na lista de processos.
a.senha ??= process.env.SENHA_PAINEL || undefined
if (!a.nome || (!a.usuario && !a.numero)) {
  console.error('Uso: npm run usuario -- --nome NOME [--usuario LOGIN --senha SENHA] [--numero WHATSAPP]')
  process.exit(1)
}
if (a.usuario && !a.senha) { console.error('Quem tem --usuario precisa de --senha.'); process.exit(1) }
if (a.senha && a.senha.length < 8) { console.error('A senha precisa de pelo menos 8 caracteres.'); process.exit(1) }

const pastaDados = path.resolve(process.env.PASTA_DADOS || 'dados')
fs.mkdirSync(pastaDados, { recursive: true })
const banco = abrirBanco(path.join(pastaDados, 'mago.db'))
const telefone = a.numero ? normalizarTelefone(a.numero) : null
const login = a.usuario?.trim().toLowerCase() || null
const existente = (telefone && banco.usuarioPorTelefone(telefone)) || (login && banco.usuarioPorLogin(login))

if (existente) {
  banco.salvarUsuario(existente.id, { nome: a.nome, telefone: telefone || existente.telefone, usuario: login || existente.usuario })
  if (a.senha) banco.salvarSenha(existente.id, hashSenha(a.senha))
  console.log(`Usuário ${a.nome} atualizado.`)
} else {
  banco.criarUsuario({ nome: a.nome, telefone, usuario: login, senha: a.senha ? hashSenha(a.senha) : null })
  console.log(`Usuário ${a.nome} criado.`)
}
