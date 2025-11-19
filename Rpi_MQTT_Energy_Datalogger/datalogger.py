import paho.mqtt.client as mqtt
import json
import csv
import os
import time
from datetime import datetime

# --- Configurações ---
BROKER_ADDRESS = "localhost"   # Broker rodando na própria RPi
BROKER_PORT = 1883
TOPIC_SUBSCRIBE = "#"          # Escuta tudo. Ajuste para "MiEnergy/#" se quiser filtrar

# Caminho completo onde os CSVs serão salvos na RPi
# ATENÇÃO: ajuste "ubuntu" para o seu usuário se for diferente
CSV_DIR = "/home/geovanne/mqtt_logs"

# Cria a pasta de logs se não existir
os.makedirs(CSV_DIR, exist_ok=True)

# --- Funções Auxiliares ---

def get_csv_filename():
    """
    Gera o nome do arquivo baseado na data atual
    (ex: 2025-11-18_energy_log.csv)
    """
    date_str = datetime.now().strftime("%Y-%m-%d")
    return os.path.join(CSV_DIR, f"{date_str}_energy_log.csv")


def flatten_json(y):
    """
    Achata o JSON (transforma objetos aninhados em linha única).
    Ex.: tensao_a: {value: 220} -> tensao_a_value: 220
    """
    out = {}

    def flatten(x, name=''):
        if isinstance(x, dict):
            for a in x:
                flatten(x[a], name + a + '_')
        elif isinstance(x, list):
            for i, a in enumerate(x):
                flatten(a, name + str(i) + '_')
        else:
            out[name[:-1]] = x

    flatten(y)
    return out


def save_to_csv(data_dict):
    """
    Escreve os dados no CSV, criando cabeçalho se necessário.
    """
    filepath = get_csv_filename()
    file_exists = os.path.isfile(filepath)

    # Timestamp local do momento em que o logger recebeu/processou
    data_dict['log_timestamp'] = datetime.now().strftime("%H:%M:%S")

    # Ordena as chaves para manter consistência nas colunas
    fieldnames = ['log_timestamp'] + sorted(
        [k for k in data_dict.keys() if k != 'log_timestamp']
    )

    try:
        # Garante encoding explícito
        with open(filepath, mode='a', newline='', encoding='utf-8') as csv_file:
            writer = csv.DictWriter(
                csv_file,
                fieldnames=fieldnames,
                extrasaction='ignore'
            )

            # Se o arquivo ainda não existia, escreve o cabeçalho
            if not file_exists:
                writer.writeheader()

            writer.writerow(data_dict)
            print(f"[GRAVADO] {filepath} -> {data_dict}")

    except Exception as e:
        print(f"[ERRO CSV] {e}")


# --- Callbacks MQTT ---

def on_connect(client, userdata, flags, rc):
    print(f"[MQTT] Conectado ao Broker! Código: {rc}")
    client.subscribe(TOPIC_SUBSCRIBE)
    print(f"[MQTT] Inscrito em: {TOPIC_SUBSCRIBE}")


def on_message(client, userdata, msg):
    try:
        payload_str = msg.payload.decode('utf-8', errors='replace')
        print(f"\n[RECEBIDO] Tópico: {msg.topic} | Payload: {payload_str}")

        # Tenta converter para JSON
        payload_json = json.loads(payload_str)

        # Achata o JSON
        flat_data = flatten_json(payload_json)

        # Adiciona info do tópico
        flat_data['mqtt_topic'] = msg.topic

        save_to_csv(flat_data)

    except json.JSONDecodeError:
        print(f"[ERRO] Payload não é um JSON válido: {msg.payload}")
    except Exception as e:
        print(f"[ERRO GERAL] {e}")


# --- Loop Principal ---

def main():
    # ID do cliente para ficar claro nos logs do broker
    client = mqtt.Client(client_id="rpi_energy_datalogger")
    client.on_connect = on_connect
    client.on_message = on_message

    print(f"[MQTT] Tentando conectar em {BROKER_ADDRESS}:{BROKER_PORT}...")

    while True:
        try:
            client.connect(BROKER_ADDRESS, BROKER_PORT, 60)
            client.loop_forever()
        except Exception as e:
            print(f"[ERRO CONEXÃO] {e}. Tentando novamente em 5s...")
            time.sleep(5)


if __name__ == "__main__":
    main()
