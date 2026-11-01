# Atendimento automático do Mago

Assistente de menu no WhatsApp do Mago, com painel de administração. Não usa inteligência artificial: o assistente só envia os textos aprovados no painel.

## O que tem aqui

| Arquivo | O que faz |
|---|---|
| `src/assistente.js` | A conversa: menu, coleta de nome, nascimento, foto e comprovante, e os comandos do Mago. |
| `src/whatsapp.js` | A ligação com o WhatsApp (biblioteca Baileys, conexão não oficial). |
| `src/painel/` | O painel: entrada por link, código ou senha, e as telas de edição. |
| `src/db.js` | O banco de dados (um arquivo SQLite) e os serviços e valores iniciais. |
| `src/textos.js` | Os textos originais de todas as mensagens. |
| `test/` | Testes automáticos (`npm test`). |

## Como funciona com um número só

O assistente roda no WhatsApp pessoal do Mago, como um "aparelho conectado", do mesmo jeito que o WhatsApp Web.

- **Pedidos e comandos** ficam na conversa do Mago com ele mesmo ("Mensagem para mim"). O Mago dita ali `pagou 12`, `assumir 12`, `liberar 12`, `valor 12 200`, `pedidos de hoje`, `painel` (recebe um link para entrar no painel) ou `ajuda`. Tudo que não começa com um comando é ignorado, então ele pode continuar usando essa conversa para anotações.
- **Contatos pessoais** (família, amigos) devem ser marcados em Painel › Contatos. O assistente nunca responde a quem está marcado.
- **Se o Mago escreve numa conversa**, o assistente fica quieto nela por 12 horas. O tempo pode ser mudado em Ajustes.
- **Grupos, status e canais** são sempre ignorados.

## Instalar num PC com Windows (o jeito fácil)

1. No PC que vai ficar ligado, abra o PowerShell: tecla Windows, escreva `powershell` e tecle Enter.
2. Cole a linha abaixo e tecle Enter:

```powershell
irm https://raw.githubusercontent.com/bibifofinhabb2012-a11y/jogos/atendimento-mago/atendimento-mago/instalador/instalar.ps1 | iex
```

3. Responda as perguntas: seu nome, usuário e senha do painel, seu WhatsApp, o WhatsApp do Mago e os dados do Pix.

O instalador baixa tudo sozinho, sem pedir administrador, e guarda em `C:\Users\<você>\AtendimentoMago`. No fim, abre o painel no navegador e explica como conectar o WhatsApp. Ficam três atalhos na área de trabalho (painel, ligar e desligar), e o assistente liga sozinho sempre que alguém entra no Windows.

- **Endereço do painel na internet.** Se você aceitar, o instalador usa o túnel gratuito da Cloudflare. O endereço muda toda vez que o PC reinicia. Por isso, o jeito de entrar de fora é o Mago ditar `painel` na conversa dele com ele mesmo: chega um link novo, ele toca e entra.
- **Atualizar.** Rode a mesma linha de novo. Pedidos, ajustes e a conexão do WhatsApp continuam como estão.
- **Problemas.** O registro fica em `AtendimentoMago\registro.txt`.
- **Cuidados.** O PC não pode dormir (o instalador oferece desligar a suspensão na tomada). Se for notebook, fechar a tampa costuma suspender. Se o Windows pedir senha ao ligar, o assistente só começa depois que alguém entra.

## Instalar num servidor Linux

É preciso um servidor ligado o tempo todo (uma VPS Linux pequena serve), com Node.js 22.13 ou mais novo. Hospedagem gratuita que desliga quando ninguém usa não funciona, porque a conexão com o WhatsApp precisa ficar aberta.

```bash
git clone -b atendimento-mago https://github.com/bibifofinhabb2012-a11y/jogos && cd jogos/atendimento-mago
npm ci
```

Crie os dois usuários do painel. Use o número com DDD, sem o +55.

```bash
npm run usuario -- --nome Ricardo --usuario ricardo --senha "troque-esta-senha" --numero 51999990000
```

```bash
npm run usuario -- --nome Mago --numero 51988880000
```

Inicie. `ENDERECO_PUBLICO` é o endereço do painel na internet, que vai dentro dos links de acesso.

```bash
ENDERECO_PUBLICO=https://painel.exemplo.com.br PORTA=3000 npm start
```

Variáveis de ambiente:

| Variável | Padrão | Para que serve |
|---|---|---|
| `PORTA` | `3000` | Porta do painel. |
| `ENDERECO_PUBLICO` | `http://localhost:PORTA` | Endereço usado nos links de acesso enviados pelo WhatsApp. |
| `PASTA_DADOS` | `./dados` | Banco, fotos, comprovantes e sessão do WhatsApp. |
| `LOG` | `info` | Nível de registro (`error`, `warn`, `info`, `debug`). |

Para o programa voltar sozinho depois de uma queda ou reinício do servidor, use o pm2:

```bash
npm install -g pm2 && ENDERECO_PUBLICO=https://painel.exemplo.com.br pm2 start npm --name mago -- start && pm2 save && pm2 startup
```

O painel precisa de HTTPS, porque o cookie de acesso e os links passam por ele. O jeito mais simples é o Caddy na frente, com um `Caddyfile` assim:

```
painel.exemplo.com.br {
    reverse_proxy localhost:3000
}
```

Depois de entrar no painel, **preencha os dados do Pix** (menu Pix). O código não traz a chave nem o titular, para esses dados não ficarem no repositório. Enquanto não forem preenchidos, o cliente vê "PREENCHER NO PAINEL" no lugar da chave.

## Conectar o WhatsApp do Mago

1. Entre no painel com usuário e senha (o link pelo WhatsApp só funciona depois que o WhatsApp estiver conectado).
2. Abra **WhatsApp** no menu do painel.
3. Digite o número do Mago e toque em **Gerar código**. Aparece um código de 8 letras, também soletrado para o TalkBack ler.
4. No celular do Mago: WhatsApp › Mais opções › Dispositivos conectados › Conectar dispositivo › **Conectar com número de telefone**, e digite o código.

Também dá para ler o QR que aparece no painel ou no terminal do servidor.

## Cópia de segurança

Tudo fica na pasta `dados/`. Copiar essa pasta é o backup completo. A subpasta `dados/whatsapp-sessao` é a conexão com o WhatsApp: quem tiver uma cópia dela consegue ler e enviar mensagens como o Mago, então guarde com o mesmo cuidado que uma senha.

## Decisões provisórias (o Mago pode mudar)

1. **Os 10 reais do trabalho espiritual não são descontados do valor do trabalho.** Se forem, basta o Mago informar o valor já descontado no comando `valor`.
2. **"Primeira consulta" vale para o número que nunca teve um pedido pago.** Começar um pedido e desistir não tira o desconto. O desconto só existe nos serviços que têm "preço na primeira consulta" preenchido no painel (hoje, só Cartas › Jogo completo).
3. **Conversa parada recomeça do menu depois de 24 horas.** Vale só até a foto. Quem já recebeu os dados do Pix pode mandar o comprovante a qualquer momento, ou digitar 0 para cancelar.
4. **Fotos e comprovantes são apagados depois de 90 dias.** O nome, a data de nascimento e o histórico do pedido continuam guardados.
5. **Depois que o Mago confirma o pagamento**, o assistente fica quieto com aquele cliente até o Mago dizer `liberar`. A mensagem de encerramento está em branco (o cliente não recebe nada), mas pode ser escrita no painel.

## O que ainda precisa ser testado com o celular de verdade

Os testes automáticos cobrem a conversa inteira, os comandos e o painel com um WhatsApp simulado. A ligação real com o WhatsApp só pode ser testada com o número conectado:

- **Notificação dos pedidos.** Mensagens que um aparelho conectado manda para a conversa "Mensagem para mim" provavelmente chegam **sem notificação** no celular do Mago. Se isso se confirmar, preencha em Ajustes um **número que recebe os pedidos** (um segundo chip, por exemplo). Os pedidos passam a chegar ali com notificação normal, e os comandos ditados desse número também são aceitos.
- **Comandos ditados.** Confirmar que o TalkBack e o teclado por voz escrevem "pagou 12" com algarismos. Se escreverem "pagou doze", o assistente não entende ainda.
- **Identificação dos clientes.** O WhatsApp está migrando para identificadores sem número de telefone (LID). O código usa o número quando o WhatsApp informa, mas vale conferir no painel, em Contatos, se os números aparecem certos.

- **Instalador do Windows.** Foi escrito sem um Windows à mão para testar. Se algum passo der erro, a mensagem diz qual foi, e rodar de novo é seguro.

## Risco do número

A conexão não é oficial. O WhatsApp pode bloquear o número, e o número é o pessoal do Mago. Para reduzir o risco, o assistente só responde quem escreveu primeiro, nunca faz envio em massa, mostra "digitando" antes de responder e não marca mensagens como lidas. Isso reduz o risco, mas não o elimina.
