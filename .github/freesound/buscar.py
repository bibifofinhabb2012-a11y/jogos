"""Busca sons no Freesound e baixa as prévias em mp3 de alta qualidade, com créditos.
Roda no GitHub Actions (a rede da sessão do Claude não alcança o Freesound).
Entrada: variável BUSCAS, uma por linha, no formato  nome | termos de busca
Saída: sons-candidatos/<nome>/<id>.mp3 e sons-candidatos/creditos.json"""
import os, sys, json, re, urllib.request, urllib.parse

CHAVE = os.environ.get('FREESOUND_API_KEY', '').strip()
if not CHAVE:
    sys.exit('Falta o segredo FREESOUND_API_KEY no repositório (Settings > Secrets and variables > Actions).')
QTD = int(os.environ.get('QUANTIDADE', '8'))
DUR = os.environ.get('DURACAO_MAX', '20')
FILTRO = f'license:("Creative Commons 0" OR "Attribution") duration:[0.2 TO {DUR}]'
CAMPOS = 'id,name,username,license,url,duration,previews,num_downloads,avg_rating,tags,description'
SAIDA = 'sons-candidatos'

def pegar(url):
    req = urllib.request.Request(url, headers={'Authorization': 'Token ' + CHAVE})
    with urllib.request.urlopen(req, timeout=60) as r:
        return r.read()

creditos = []
for linha in os.environ.get('BUSCAS', '').splitlines():
    if '|' not in linha:
        continue
    nome, termos = [p.strip() for p in linha.split('|', 1)]
    nome = re.sub(r'[^a-z0-9-]+', '-', nome.lower()).strip('-')
    q = urllib.parse.urlencode({'query': termos, 'filter': FILTRO, 'fields': CAMPOS,
                                'page_size': QTD, 'sort': 'score'})
    try:
        res = json.loads(pegar('https://freesound.org/apiv2/search/text/?' + q))
    except Exception as e:
        sys.exit(f'Erro na busca "{termos}": {e}')
    os.makedirs(f'{SAIDA}/{nome}', exist_ok=True)
    print(f'{nome}: {res.get("count", 0)} resultados, baixando {len(res.get("results", []))}')
    for s in res.get('results', []):
        arq = f'{SAIDA}/{nome}/{s["id"]}.mp3'
        try:
            open(arq, 'wb').write(pegar(s['previews']['preview-hq-mp3']))
        except Exception as e:
            print(f'  falhou {s["id"]}: {e}'); continue
        creditos.append({'grupo': nome, 'busca': termos, 'arquivo': arq, 'id': s['id'],
                         'titulo': s['name'], 'autor': s['username'], 'licenca': s['license'],
                         'link': s['url'], 'duracao': round(s['duration'], 2),
                         'downloads': s['num_downloads'], 'nota': s['avg_rating'],
                         'tags': s['tags'][:12], 'descricao': (s.get('description') or '')[:300]})
        print(f'  {s["id"]} {s["duration"]:.1f}s {s["username"]}: {s["name"]}')

anterior = []
if os.path.exists(f'{SAIDA}/creditos.json'):
    anterior = json.load(open(f'{SAIDA}/creditos.json'))
ids = {c['id'] for c in creditos}
json.dump([c for c in anterior if c['id'] not in ids] + creditos,
          open(f'{SAIDA}/creditos.json', 'w'), ensure_ascii=False, indent=1)
print(f'Total baixado agora: {len(creditos)}')
