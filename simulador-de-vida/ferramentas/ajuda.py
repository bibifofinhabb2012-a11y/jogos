"""Gera a ajuda do jogo (F1, em src/jogo.cpp) e a lista de teclas do LEIA-ME.txt a partir de uma lista só.
Uso: python3 ferramentas/ajuda.py"""
import os
RAIZ = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
TECLAS = [
 ("Seta para cima", "andar para a frente; segure para continuar; com Control, correr"),
 ("Seta para baixo", "andar de costas"),
 ("Setas para a esquerda e para a direita", "andar de lado"),
 ("Control mais seta para o lado", "virar 45 graus"),
 ("Shift mais seta para o lado", "virar 90 graus"),
 ("Shift mais seta para cima ou para baixo", "dar meia-volta"),
 ("R", "sentar, no chão ou no sofá, na poltrona, no vaso ou na cama que estiver na sua frente; em outra posição, levantar"),
 ("Control mais R", "deitar; de frente para a cama, deita nela e dorme. Dormindo, as teclas não acordam: acorda sozinho com a energia em 1000, ou de manhã na cama; U, S, E, H, V, W e Q continuam respondendo"),
 ("Shift mais R", "agachar"),
 ("Shift mais Enter", "abrir ou fechar a porta, a porteira ou a porta do box encostada em você"),
 ("Espaço", "usa o que está na mão selecionada: comer, beber, tomar café, olhar a bússola, entrar no saco de dormir e dormir (deitado, em qualquer lugar: fazenda, cidade, fazenda dos outros), contar o dinheiro. Na roça: machado limpa o matagal da frente, semente planta na terra limpa, regador rega a planta da frente e enche na beira do córrego. No curral de corte: com a faca, abate a vaca de corte mais perto, até 5 passos (ela pode dar coice), e dá carne e couro. No curral de leite: garrafa vazia de frente para uma vaca, tira leite, uma vez por dia cada vaca. Garrafa de leite: bebe um gole. Na cozinha: carne crua de frente para o fogão aceso, assa. Com a mão vazia: de frente para um móvel, mexe nele; de frente para seu Tião, dona Nezinha, seu Zé (no balcão do bar) ou o prefeito, conversa e compra; de frente para o trator, escolhe para onde ir; de dentro do box, de frente para o chuveiro, liga e desliga; de frente para uma planta pronta, colhe (o arroz se colhe com o machado); de frente para uma vaca, faz carinho; de frente para uma árvore do pomar, descreve"),
 ("Control mais Espaço", "segura o móvel da frente para arrastar; ande para empurrar ou puxar; de novo, solta"),
 ("Z", "pular: segure para pegar impulso, um bip por nível, até 10 de altura, e solte para pular; com Z segurado, uma seta escolhe a direção do pulo, e ele leva até 5 passos para lá; dentro de casa, o teto segura em 2"),
 ("B", "onde você está"),
 ("Shift mais B", "onde você está, a que distância das divisas e o que tem à frente"),
 ("Ç", "próxima categoria: móveis, pessoas (também os outros jogadores, no online), portas e porteiras, lugares, plantação, animais, objetos (as árvores do pomar)"),
 ("Shift mais Ç", "categoria anterior"),
 ("L", "próximo item da categoria, do mais perto para o mais longe"),
 ("K", "item anterior"),
 ("O", "rastreia o item escolhido: um bip soa de onde ele está; O de novo desliga"),
 ("Alt mais O", "anda sozinho até o item, abrindo as portas; qualquer tecla para"),
 ("Q", "dia e hora"),
 ("I", "abre e fecha a mochila; com ela aberta, setas para cima e para baixo escolhem o item, uma letra pula para o próximo item com aquela inicial, seta para a direita põe na mão direita, seta para a esquerda põe na mão esquerda, Esc fecha"),
 ("1", "seleciona a mão esquerda e diz o que tem nela; apertado duas vezes, esvazia"),
 ("2", "o mesmo para a mão direita"),
 ("3", "diz o que tem nas duas mãos"),
 ("T", "combina o que está nas duas mãos (isqueiro e cigarro: tenta acender; isqueiro e latinha de gás: enche o isqueiro; couro e faca: faz cinto, bolsa, chapéu, botas ou sela); com as duas vazias, bate palmas"),
 ("U", "fome, de 0 a 1000"), ("S", "sede: água no corpo, de 0 a 1000"), ("E", "energia, de 0 a 1000"), ("H", "saúde, de 0 a 1000"), ("W", "limpeza: se precisa de banho"),
 ("V", "intestino"), ("Shift mais V", "bexiga"),
 ("Control mais Q, segurando", "fazer cocô, sentando, até acabar"),
 ("Shift mais Q, segurando", "fazer xixi, de cócoras, até acabar"),
 ("Dois-pontos", "no jogo online, abre o campo de mensagem do chat; Enter envia, Esc cancela, Backspace apaga; o jogo fala cada letra que você digita"),
 ("Tab", "menu: tutorial, hospedar online, entrar em jogo online, jogadores, teleportar para o evento (quando tem casamento), conversar sobre a fazenda (depois de casar), ativar e desativar chat, histórico do chat, ativar e desativar voz, configurações de volume, sair; Tab de novo volta ao jogo; sozinho, o menu pausa o jogo, no online não"),
 ("Nos menus", "setas para cima e para baixo escolhem, Enter faz, Shift Enter pega, guarda ou vende tudo, setas para os lados mudam volume, uma letra pula, Esc volta"),
 ("F2", "repete a última fala"), ("F1", "esta ajuda"), ("Esc duas vezes", "salva e sai"),
]
txt = "Teclas. " + " ".join(f"{k}: {v}." for k, v in TECLAS)
P = os.path.join(RAIZ, "src", "jogo.cpp"); s = open(P, encoding="utf-8").read()
i = s.index('void Jogo::ajuda() {'); j = s.index('\n}\n', i) + 3
s = s[:i] + 'void Jogo::ajuda() {\n    fala::dizer("' + txt.replace('"', '\\"') + '");\n}\n' + s[j:]
open(P, "w", encoding="utf-8").write(s)
P = os.path.join(RAIZ, "LEIA-ME.txt"); s = open(P, encoding="utf-8").read()
i = s.index('TECLAS'); j = s.index('\n\n', i)
s = s[:i] + "TECLAS\n" + "\n".join(f"{k}: {v}." for k, v in TECLAS) + s[j:]
open(P, "w", encoding="utf-8").write(s)
print("ajuda e LEIA-ME atualizados")
