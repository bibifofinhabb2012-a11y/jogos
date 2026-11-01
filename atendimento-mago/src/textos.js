// Todas as mensagens que o assistente envia. O painel permite editar cada uma.
// Uma mensagem deixada em branco no painel não é enviada.

const PIX = '{pix_tipo}, {pix_chave}, {pix_titular}, {pix_banco}'

export const TEXTOS = [
  {
    chave: 'menu_principal',
    rotulo: 'Menu principal',
    ajuda: '{opcoes} é a lista numerada de opções.',
    padrao: 'Olá! Aqui é o atendimento automático do Mago. Responda com o número da opção:\n\n{opcoes}\n\nA qualquer momento, digite 0 para voltar a este menu.'
  },
  {
    chave: 'submenu',
    rotulo: 'Submenu (oráculos, tipos de jogo, linhas de trabalho)',
    ajuda: '{titulo} é o texto do item no painel de serviços. {opcoes} é a lista numerada.',
    padrao: '{titulo}\n\n{opcoes}\n\n0. Voltar ao menu principal'
  },
  {
    chave: 'indisponivel',
    rotulo: 'Serviço indisponível',
    ajuda: '{servico} é o nome do serviço.',
    padrao: '{servico} está indisponível no momento. Voltando ao menu.'
  },
  {
    chave: 'nao_entendi',
    rotulo: 'Não entendi (antes de repetir as opções)',
    ajuda: 'Enviada quando o cliente manda áudio, figurinha ou um número que não existe.',
    padrao: 'Não entendi. Responda só com o número da opção.'
  },
  {
    chave: 'pedir_nome',
    rotulo: 'Pedir nome completo',
    ajuda: '{servico} é o serviço escolhido. {valor} é o preço.',
    padrao: 'Você escolheu: {servico}, {valor}.\n\nPara continuar, escreva seu nome completo.'
  },
  {
    chave: 'nome_invalido',
    rotulo: 'Nome incompleto',
    ajuda: '',
    padrao: 'Preciso do seu nome completo, com nome e sobrenome.'
  },
  {
    chave: 'pedir_nascimento',
    rotulo: 'Pedir data de nascimento',
    ajuda: '{primeiro_nome} é o primeiro nome do cliente.',
    padrao: 'Obrigado, {primeiro_nome}. Agora escreva sua data de nascimento, por exemplo 03/05/1990.'
  },
  {
    chave: 'nascimento_invalido',
    rotulo: 'Data de nascimento não entendida',
    ajuda: '',
    padrao: 'Não consegui entender a data. Escreva dia, mês e ano, por exemplo 03/05/1990.'
  },
  {
    chave: 'pedir_foto',
    rotulo: 'Pedir foto (com aviso de privacidade)',
    ajuda: '{dias} é o número de dias até a foto ser apagada, definido em Ajustes.',
    padrao: 'Agora envie uma foto sua, de rosto.\n\nSua foto e seus dados são usados só para o seu atendimento. A foto e o comprovante são apagados depois de {dias} dias.'
  },
  {
    chave: 'foto_invalida',
    rotulo: 'Foto não recebida',
    ajuda: '',
    padrao: 'Preciso de uma foto sua. Toque no clipe ou na câmera e envie a foto.'
  },
  {
    chave: 'resumo',
    rotulo: 'Resumo do pedido e dados do Pix',
    ajuda: '{pedido}, {servico}, {valor}, {nome}, {nascimento}, e os dados do Pix: ' + PIX + '. Logo depois, a chave Pix é enviada sozinha numa mensagem, para o cliente copiar.',
    padrao: 'Resumo do pedido número {pedido}\nServiço: {servico}\nValor: {valor}\nNome: {nome}\nNascimento: {nascimento}\n\nPara pagar, faça um Pix:\nTipo de chave: {pix_tipo}\nChave: {pix_chave}\nTitular: {pix_titular}\nBanco: {pix_banco}\n\nDepois envie aqui o comprovante, em foto ou PDF. A chave vai na próxima mensagem, para você copiar.'
  },
  {
    chave: 'valor_sob_consulta',
    rotulo: 'Como o valor aparece nos serviços sob consulta',
    ajuda: '{taxa} é o preço do serviço, que é a taxa para enviar o pedido.',
    padrao: '{taxa} para enviar o pedido. O valor do trabalho o Mago informa depois'
  },
  {
    chave: 'aviso_primeira',
    rotulo: 'Aviso de preço de primeira consulta',
    ajuda: 'Vai logo depois do valor.',
    padrao: ' (preço de primeira consulta)'
  },
  {
    chave: 'comprovante_invalido',
    rotulo: 'Comprovante não recebido',
    ajuda: '',
    padrao: 'Preciso do comprovante do Pix, em foto ou PDF. Para cancelar, digite 0.'
  },
  {
    chave: 'comprovante_recebido',
    rotulo: 'Comprovante recebido',
    ajuda: '{pedido} é o número do pedido.',
    padrao: 'Comprovante recebido! O Mago vai conferir o pagamento e confirma por aqui. Seu pedido é o número {pedido}.'
  },
  {
    chave: 'aguardando_mago',
    rotulo: 'Cliente escreve enquanto o Mago confere',
    ajuda: 'Enviada uma vez só. {pedido} é o número do pedido.',
    padrao: 'Seu pedido número {pedido} já está com o Mago. Assim que ele conferir, você recebe a confirmação aqui. Para fazer outro pedido, digite 0.'
  },
  {
    chave: 'pagamento_confirmado',
    rotulo: 'Pagamento confirmado',
    ajuda: '{pedido} é o número do pedido.',
    padrao: 'Pagamento confirmado! Seu pedido número {pedido} está com o Mago, e ele continua o atendimento por aqui.'
  },
  {
    chave: 'valor_trabalho',
    rotulo: 'Valor do trabalho espiritual',
    ajuda: '{pedido}, {valor} e os dados do Pix: ' + PIX + '.',
    padrao: 'O Mago avaliou seu pedido número {pedido}. O valor do trabalho é {valor}.\n\nPara pagar, faça um Pix:\nTipo de chave: {pix_tipo}\nChave: {pix_chave}\nTitular: {pix_titular}\nBanco: {pix_banco}\n\nDepois envie aqui o comprovante, em foto ou PDF.'
  },
  {
    chave: 'atendimento_encerrado',
    rotulo: 'Quando o Mago libera o cliente',
    ajuda: 'Em branco por padrão: o cliente não recebe nada. {pedido} é o número do pedido.',
    padrao: ''
  },
  {
    chave: 'mago_novo_pedido',
    rotulo: 'Para o Mago: novo pedido',
    ajuda: '{pedido}, {nome}, {nascimento} por extenso, {servico} com preço e aviso de primeira consulta.',
    padrao: 'Novo pedido número {pedido}. Nome: {nome}. Nascimento: {nascimento}. Serviço: {servico}. Comprovante e foto a seguir.'
  },
  {
    chave: 'mago_comprovante_trabalho',
    rotulo: 'Para o Mago: comprovante do valor do trabalho',
    ajuda: '{pedido}, {nome}, {valor}.',
    padrao: 'Comprovante do trabalho, pedido número {pedido}. Nome: {nome}. Valor: {valor}. Comprovante a seguir.'
  }
]

export const TEXTOS_POR_CHAVE = Object.fromEntries(TEXTOS.map(t => [t.chave, t]))
