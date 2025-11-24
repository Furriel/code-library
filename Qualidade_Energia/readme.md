# 📊 Análise de Qualidade de Energia — IF Goiano

Repositório contendo o script em Python utilizado para analisar dados elétricos reais obtidos em um prédio de laboratórios do **Instituto Federal Goiano — Campus Trindade**, com foco em **qualidade da energia**, **eficiência energética** e **conformidade normativa** conforme o **PRODIST/ANEEL**.

O projeto faz parte de um Trabalho de Conclusão de Curso e integra medições reais, tratamento estatístico, geração de gráficos e diagnóstico técnico.

---

## ✅ Objetivos do Projeto

- Realizar diagnóstico energético baseado em dados reais
- Avaliar a qualidade da energia elétrica fornecida ao edifício
- Verificar conformidade com o PRODIST — Módulo 8
- Identificar oportunidades de melhoria e riscos operacionais
- Automatizar a análise para replicação futura

---

## 📁 Estrutura do Repositório

```

├── analise_qualidade_energia.py   # Script principal
├── IF.csv                         # Arquivo de dados (exemplo)
├── README.md                      # Documentação
│
└── resultados_qualidade_energia/  # Gerado automaticamente
├── grafico_tensao_rms.png
├── grafico_thd_tensao.png
├── grafico_fator_potencia.png
├── grafico_frequencia.png
├── hist_tensao_L1.png
├── hist_thd_L1.png
├── hist_fp.png
├── hist_freq.png
├── tensao_diaria.csv
├── thdv_diaria.csv
├── fp_diaria.csv
├── freq_diaria.csv
├── eventos_thdv_acima_10.csv
└── eventos_fp_abaixo_092.csv

````

A pasta `resultados_qualidade_energia` é criada automaticamente ao executar o script.

---

## 🧠 O que o script faz?

✅ Carrega e organiza o CSV do analisador de energia  
✅ Gera estatísticas completas:

- Tensão RMS por fase  
- THD de tensão por fase  
- Fator de potência total  
- Frequência elétrica  
- Desequilíbrio entre fases  

✅ Detecta eventos críticos:

- THDv acima de 10% (limite PRODIST)
- |FP| abaixo de 0,92 (não conformidade)

✅ Gera gráficos temporais e histogramas  
✅ Salva tabelas prontas para LaTeX/TCC  
✅ Imprime resumo interpretativo no terminal

---

## 📦 Requisitos

- Python **3.9+**
- Bibliotecas:

```bash
pip install pandas numpy matplotlib
````

---

## ▶️ Como executar

1. Clone o repositório:

```bash
git clone https://github.com/usuario/analise-qualidade-energia.git
cd analise-qualidade-energia
```

2. Garanta que o arquivo `IF.csv` está na pasta
3. Execute o script:

```bash
python analise_qualidade_energia.py
```

4. Confira gráficos e tabelas gerados em:

```
resultados_qualidade_energia/
```

---

## 📄 Formato esperado do CSV

O script assume colunas padronizadas como:

```
TIME;
VRMS(V) L1 AVG;
VRMS(V) L2 AVG;
VRMS(V) L3 AVG;
VTHD L1 MAX;
VTHD L2 MAX;
VTHD L3 MAX;
TPF ALL AVG;
FREQ AVG;
```

* `TIME` deve estar em formato datetime
* Separador padrão: `;`

---

## 📐 Limites de referência (PRODIST — Módulo 8)

| Parâmetro                   | Limite         |
| --------------------------- | -------------- |
| Tensão em regime permanente | 0,93 a 1,05 pu |
| THDv — Baixa Tensão         | ≤ 10%          |
| Fator de potência           | ≥ 0,92         |
| Frequência                  | 60 Hz ± 1%     |

Esses limites são automaticamente aplicados na análise.

---

## 📊 Resultados gerados

* Estatísticas gerais e por dia
* Gráficos de tendência
* Histogramas estatísticos
* Arquivos CSV organizados para relatório técnico
* Detecção automática de anomalias

Ideal para:

✅ TCCs
✅ Auditorias energéticas
✅ Projetos de eficiência energética
✅ Estudos de cargas não lineares
✅ Monitoramento institucional

---

## ⚠️ Limitações

* Dados representam apenas um ponto da instalação
* Campanha curta pode não capturar sazonalidade
* Não realiza análise tarifária ou demanda contratada
* Não identifica origem física dos harmônicos

---

## 🔄 Expansões futuras sugeridas

* Dashboard em Streamlit/Plotly
* Suporte a múltiplos arquivos CSV
* Relatório automático em PDF
* Cálculo de consumo e perdas estimadas
* Correlação com ocupação ou calendário acadêmico

---

## 📚 Citação sugerida

Se utilizar este código em TCC, artigo ou relatório:

```
Furriel, G. (2025). Diagnóstico Energético e Avaliação da Qualidade da Energia em Edificação Educacional Pública. Instituto Federal Goiano – Campus Trindade.
```

---

## 🤝 Contribuições

Pull requests, issues e melhorias são bem-vindas!
Se quiser enviar novos datasets para análise, abra uma issue.

---

## 🧑‍💻 Autor

Pesquisa acadêmica aplicada desenvolvida no
**Instituto Federal Goiano — Campus Trindade**


## 📬 Contato

📧 [geovanne.furriel@ifgoiano.edu.br](mailto:geovanne.furriel@ifgoiano.edu.br)
🌎 [https://www.ifgoiano.edu.br](https://www.ifgoiano.edu.br)

---

## ✅ Licença

* `CC BY 4.0` — ideal para contexto acadêmico
