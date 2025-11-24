#!/usr/bin/env python3
# ============================================================
# ANÁLISE DE QUALIDADE DE ENERGIA - IF GOIANO
# A partir de arquivo CSV (formato IF.csv)
# ============================================================

import os
from pathlib import Path

import numpy as np
import pandas as pd
import matplotlib.pyplot as plt

# ------------------------------------------------------------
# CONFIGURAÇÕES GERAIS
# ------------------------------------------------------------
# Caminho do arquivo CSV de entrada
CSV_PATH = "IF.csv"          # ajuste se necessário

# Pasta de saída para gráficos e tabelas
OUTPUT_DIR = Path("resultados_qualidade_energia")
OUTPUT_DIR.mkdir(exist_ok=True)

# ------------------------------------------------------------
# FUNÇÕES AUXILIARES
# ------------------------------------------------------------
def carregar_dados(csv_path: str) -> pd.DataFrame:
    """
    Carrega o CSV no padrão do analisador, ajusta o separador,
    converte a coluna TIME para datetime e cria coluna de data (DATE).
    """
    df = pd.read_csv(csv_path, sep=';')
    # Ajustar nome da coluna de tempo, se necessário
    if 'TIME' not in df.columns:
        raise ValueError("Coluna 'TIME' não encontrada no CSV.")
    df['TIME'] = pd.to_datetime(df['TIME'], dayfirst=True, errors='coerce')
    df = df.dropna(subset=['TIME']).copy()
    df['DATE'] = df['TIME'].dt.date
    return df


def salvar_fig(nome_arquivo: str):
    """Salva o gráfico atual na pasta de saída com DPI adequado."""
    plt.tight_layout()
    plt.savefig(OUTPUT_DIR / nome_arquivo, dpi=300)
    plt.close()


# ------------------------------------------------------------
# 1) ANÁLISE GERAL - TENSÃO, THD, FP, FREQUÊNCIA
# ------------------------------------------------------------
def analise_geral(df: pd.DataFrame) -> dict:
    """
    Calcula estatísticas globais (média, min, max) para:
    - Tensão RMS por fase
    - THD de tensão por fase
    - Fator de potência total
    - Frequência
    Também calcula o desequilíbrio médio de tensão.
    """
    resultados = {}

    # ---------- TENSÃO ----------
    tens_cols = ['VRMS(V) L1 AVG', 'VRMS(V) L2 AVG', 'VRMS(V) L3 AVG']
    for col in tens_cols:
        if col not in df.columns:
            raise ValueError(f"Coluna '{col}' não encontrada no CSV.")
    tens_stats = {}
    for col in tens_cols:
        tens_stats[col] = {
            "mean": df[col].mean(),
            "min": df[col].min(),
            "max": df[col].max(),
        }
    # Desequilíbrio médio (entre as fases)
    v1 = df['VRMS(V) L1 AVG'].mean()
    v2 = df['VRMS(V) L2 AVG'].mean()
    v3 = df['VRMS(V) L3 AVG'].mean()
    v_med = np.mean([v1, v2, v3])
    desequilibrio = (max(v1, v2, v3) - min(v1, v2, v3)) / v_med * 100

    resultados['tensao'] = tens_stats
    resultados['desequilibrio_tensao_percent'] = desequilibrio

    # ---------- THD DE TENSÃO ----------
    thd_cols = ['VTHD L1 MAX', 'VTHD L2 MAX', 'VTHD L3 MAX']
    for col in thd_cols:
        if col not in df.columns:
            raise ValueError(f"Coluna '{col}' não encontrada no CSV.")
    thd_stats = {}
    for col in thd_cols:
        thd_stats[col] = {
            "mean": df[col].mean(),
            "p95": np.percentile(df[col], 95),
            "max": df[col].max(),
        }
    resultados['thdv'] = thd_stats

    # ---------- FATOR DE POTÊNCIA ----------
    pf_col = 'TPF ALL AVG'
    if pf_col not in df.columns:
        raise ValueError("Coluna 'TPF ALL AVG' não encontrada no CSV.")
    pf_stats = {
        "mean": df[pf_col].mean(),
        "min": df[pf_col].min(),
        "max": df[pf_col].max(),
        "mean_abs": df[pf_col].abs().mean(),
    }
    resultados['fp_total'] = pf_stats

    # ---------- FREQUÊNCIA ----------
    freq_col = 'FREQ AVG'
    if freq_col not in df.columns:
        raise ValueError("Coluna 'FREQ AVG' não encontrada no CSV.")
    freq_stats = {
        "mean": df[freq_col].mean(),
        "min": df[freq_col].min(),
        "max": df[freq_col].max(),
    }
    resultados['freq'] = freq_stats

    return resultados


# ------------------------------------------------------------
# 2) ESTATÍSTICAS DIÁRIAS (TENSÃO, THD, FP, FREQ)
# ------------------------------------------------------------
def analise_diaria(df: pd.DataFrame) -> dict:
    """
    Calcula estatísticas diárias para:
    - Tensão RMS por fase (média, mínimo, máximo)
    - THD de tensão por fase (média, máximo)
    - Fator de potência total (média, mínimo, máximo, média do módulo)
    - Frequência (média, mínimo, máximo)
    """
    daily = {}

    # Tensão
    tens_cols = ['VRMS(V) L1 AVG', 'VRMS(V) L2 AVG', 'VRMS(V) L3 AVG']
    daily_voltage = df.groupby('DATE')[tens_cols].agg(['mean', 'min', 'max'])
    daily['tensao'] = daily_voltage

    # THD tensão
    thd_cols = ['VTHD L1 MAX', 'VTHD L2 MAX', 'VTHD L3 MAX']
    daily_thd = df.groupby('DATE')[thd_cols].agg(['mean', 'max'])
    daily['thdv'] = daily_thd

    # Fator de potência
    daily_pf = df.groupby('DATE')['TPF ALL AVG'].agg(
        mean='mean',
        min='min',
        max='max'
    )
    daily_pf['mean_abs'] = df.groupby('DATE')['TPF ALL AVG'].apply(
        lambda x: np.mean(np.abs(x))
    )
    daily['fp'] = daily_pf

    # Frequência
    daily_freq = df.groupby('DATE')['FREQ AVG'].agg(['mean', 'min', 'max'])
    daily['freq'] = daily_freq

    return daily


# ------------------------------------------------------------
# 3) DETECÇÃO DE EVENTOS CRÍTICOS (THDv > 10% e FP < 0.92)
# ------------------------------------------------------------
def detectar_eventos(df: pd.DataFrame):
    """Retorna data/hora de eventos críticos: THDv > 10% e |FP| < 0.92."""
    eventos = {}

    # Eventos de THDv acima de 10% em qualquer fase
    mask_thd = (df['VTHD L1 MAX'] > 10) | (df['VTHD L2 MAX'] > 10) | (df['VTHD L3 MAX'] > 10)
    eventos_thd = df.loc[mask_thd, ['TIME', 'VTHD L1 MAX', 'VTHD L2 MAX', 'VTHD L3 MAX']]
    eventos['thdv_acima_limite'] = eventos_thd

    # Amostras com |FP| < 0.92 (não conformidade ANEEL)
    mask_fp = df['TPF ALL AVG'].abs() < 0.92
    eventos_fp = df.loc[mask_fp, ['TIME', 'TPF ALL AVG']]
    eventos['fp_abaixo_092'] = eventos_fp

    return eventos


# ------------------------------------------------------------
# 4) GERAÇÃO DOS GRÁFICOS PRINCIPAIS
# ------------------------------------------------------------
def gerar_graficos_temporais(df: pd.DataFrame):
    """Gera os 4 gráficos principais usados no laudo e salva em PNG."""

    # 1) Tensão RMS por fase
    plt.figure()
    plt.plot(df['TIME'], df['VRMS(V) L1 AVG'], label='L1')
    plt.plot(df['TIME'], df['VRMS(V) L2 AVG'], label='L2')
    plt.plot(df['TIME'], df['VRMS(V) L3 AVG'], label='L3')
    plt.xlabel('Tempo')
    plt.ylabel('Tensão [V]')
    plt.title('Tensão RMS por fase')
    plt.legend()
    salvar_fig("grafico_tensao_rms.png")

    # 2) THD de tensão
    plt.figure()
    plt.plot(df['TIME'], df['VTHD L1 MAX'], label='L1')
    plt.plot(df['TIME'], df['VTHD L2 MAX'], label='L2')
    plt.plot(df['TIME'], df['VTHD L3 MAX'], label='L3')
    plt.axhline(10, linestyle='--', label='Limite PRODIST 10%')
    plt.xlabel('Tempo')
    plt.ylabel('THD de tensão [%]')
    plt.title('THD de tensão (máximo por intervalo)')
    plt.legend()
    salvar_fig("grafico_thd_tensao.png")

    # 3) Fator de potência total
    plt.figure()
    plt.plot(df['TIME'], df['TPF ALL AVG'])
    plt.xlabel('Tempo')
    plt.ylabel('Fator de potência total')
    plt.title('Fator de potência total - trifásico')
    salvar_fig("grafico_fator_potencia.png")

    # 4) Frequência
    plt.figure()
    plt.plot(df['TIME'], df['FREQ AVG'])
    plt.xlabel('Tempo')
    plt.ylabel('Frequência [Hz]')
    plt.title('Frequência ao longo da campanha')
    salvar_fig("grafico_frequencia.png")


def gerar_histogramas(df: pd.DataFrame):
    """Gera histogramas das principais grandezas (L1, THD L1, FP, Freq)."""

    # Histograma tensão L1
    plt.figure()
    plt.hist(df['VRMS(V) L1 AVG'], bins=25)
    plt.xlabel('Tensão L1 [V]')
    plt.ylabel('Frequência')
    plt.title('Histograma - Tensão RMS L1')
    salvar_fig("hist_tensao_L1.png")

    # Histograma THD L1
    plt.figure()
    plt.hist(df['VTHD L1 MAX'], bins=25)
    plt.xlabel('THD L1 [%]')
    plt.ylabel('Frequência')
    plt.title('Histograma - THD de Tensão L1')
    salvar_fig("hist_thd_L1.png")

    # Histograma FP total
    plt.figure()
    plt.hist(df['TPF ALL AVG'], bins=25)
    plt.xlabel('Fator de potência total')
    plt.ylabel('Frequência')
    plt.title('Histograma - Fator de Potência Total')
    salvar_fig("hist_fp.png")

    # Histograma frequência
    plt.figure()
    plt.hist(df['FREQ AVG'], bins=25)
    plt.xlabel('Frequência [Hz]')
    plt.ylabel('Frequência')
    plt.title('Histograma - Frequência Elétrica')
    salvar_fig("hist_freq.png")


# ------------------------------------------------------------
# 5) FUNÇÃO PRINCIPAL
# ------------------------------------------------------------
def main():
    print("=== ANÁLISE DE QUALIDADE DE ENERGIA - IF GOIANO ===")
    print(f"Lendo arquivo CSV em: {CSV_PATH}")
    df = carregar_dados(CSV_PATH)
    print(f"Registros carregados: {len(df)}")
    print(f"Período: {df['TIME'].min()}  ->  {df['TIME'].max()}")
    print()

    # 1) Análise geral
    print(">>> Estatísticas gerais:")
    res_geral = analise_geral(df)
    # Tensões
    print("\n-- TENSÃO RMS (média / min / max) --")
    for fase, stats in res_geral['tensao'].items():
        print(f"{fase}: média={stats['mean']:.2f} V, min={stats['min']:.2f} V, max={stats['max']:.2f} V")
    print(f"\nDesequilíbrio médio de tensão: {res_geral['desequilibrio_tensao_percent']:.2f} %")

    # THDv
    print("\n-- THD DE TENSÃO (média / p95 / max) --")
    for fase, stats in res_geral['thdv'].items():
        print(f"{fase}: média={stats['mean']:.2f} %, p95={stats['p95']:.2f} %, max={stats['max']:.2f} %")

    # Fator de potência
    pf_stats = res_geral['fp_total']
    print("\n-- FATOR DE POTÊNCIA TOTAL --")
    print(f"Média={pf_stats['mean']:.3f}, min={pf_stats['min']:.3f}, "
          f"max={pf_stats['max']:.3f}, |média|={pf_stats['mean_abs']:.3f}")

    # Frequência
    freq_stats = res_geral['freq']
    print("\n-- FREQUÊNCIA --")
    print(f"Média={freq_stats['mean']:.3f} Hz, min={freq_stats['min']:.3f} Hz, "
          f"max={freq_stats['max']:.3f} Hz")

    # 2) Estatísticas diárias (salvar em CSV para usar no TCC)
    print("\n>>> Estatísticas diárias (salvas em CSV na pasta de resultados)...")
    daily = analise_diaria(df)
    daily['tensao'].to_csv(OUTPUT_DIR / "tensao_diaria.csv", sep=';', decimal=',')
    daily['thdv'].to_csv(OUTPUT_DIR / "thdv_diaria.csv", sep=';', decimal=',')
    daily['fp'].to_csv(OUTPUT_DIR / "fp_diaria.csv", sep=';', decimal=',')
    daily['freq'].to_csv(OUTPUT_DIR / "freq_diaria.csv", sep=';', decimal=',')

    # 3) Eventos críticos
    print("\n>>> Detecção de eventos críticos...")
    eventos = detectar_eventos(df)
    print(f"Eventos com THDv > 10%: {len(eventos['thdv_acima_limite'])} amostras.")
    print(f"Instantes com |FP| < 0,92: {len(eventos['fp_abaixo_092'])} amostras.")

    eventos['thdv_acima_limite'].to_csv(OUTPUT_DIR / "eventos_thdv_acima_10.csv", sep=';', decimal=',', index=False)
    eventos['fp_abaixo_092'].to_csv(OUTPUT_DIR / "eventos_fp_abaixo_092.csv", sep=';', decimal=',', index=False)

    # 4) Gráficos temporais
    print("\n>>> Gerando gráficos temporais...")
    gerar_graficos_temporais(df)
    print("Gráficos salvos em:", OUTPUT_DIR)

    # 5) Histogramas
    print("\n>>> Gerando histogramas...")
    gerar_histogramas(df)
    print("Histogramas salvos em:", OUTPUT_DIR)

    print("\n=== ANÁLISE CONCLUÍDA ===")


if __name__ == "__main__":
    main()
