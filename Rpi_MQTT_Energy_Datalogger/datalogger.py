import paho.mqtt.client as mqtt
import json
import csv
import os
import time
from datetime import datetime

# --- Configurações ---
BROKER_ADDRESS = "localhost"  # Como vai rodar na mesma Rasp que o broker
BROKER_PORT = 1883
TOPIC_SUBSCRIBE = "#"         # Escuta tudo. Ajuste para "sensores/#" se quiser filtrar
CSV_DIR = "logs"              # Pasta onde os CSVs serão salvos

# Cria a pasta de logs se não existir
if not os.path.exists(CSV_DIR):
    os.makedirs(CSV_DIR)

# --- Funções Auxiliares ---

def get_csv_filename():
    """Gera o nome do arquivo baseado na data atual (ex: 2025-11-18_energy_log.csv)"""
    date_str = datetime.now().strftime("%Y-%m-%d")
    return os.path.join(CSV_DIR, f"{date_str}_energy_log.csv")

def flatten_json(y):
    """Achata o JSON (transforma objetos aninhados em linha única) - Igual fazia no C++"""
    out = {}
    def flatten(x, name=''):
        if type(x) is dict:
            for a in x:
                flatten(x[a], name + a + '_')
        elif type(x) is list:
            for i, a in enumerate(x):
                flatten(a, name + str(i) + '_')
        else:
            out[name[:-1]] = x
    flatten(y)
    return out

def save_to_csv(data_dict):
    """Escreve os dados no CSV, criando cabeçalho se necessário"""
    filepath = get_csv_filename()
    file_exists = os.path.isfile(filepath)
    
    # Adiciona timestamp de processamento
    data_dict['log_timestamp'] = datetime.now().strftime("%H:%M:%S")
    
    # Ordena as chaves para manter consistência nas colunas
    fieldnames = ['log_timestamp'] + sorted([k for k in data_dict.keys() if k != 'log_timestamp'])

    try:
        with open(filepath, mode='a', newline='') as csv_file:
            writer = csv.DictWriter(csv_file, fieldnames=fieldnames, extrasaction='ignore')
            
            if not file_exists:
                writer.writeheader()
            
            # Se o arquivo já existe, verificamos se as colunas bateriam (opcional, aqui simplificado)
            # Nota: O DictWriter do Python ignora campos extras se extrasaction='ignore'
            # mas para colunas novas dinâmicas, o ideal seria recriar o header. 
            # Para simplicidade, assumimos estrutura constante por dia.
            writer.writerow(data_dict)
            print(f"[GRAVADO] {data_dict}")
            
    except Exception as e:
        print(f"[ERRO CSV] {e}")

# --- Callbacks MQTT ---

def on_connect(client, userdata, flags, rc):
    print(f"Conectado ao Broker! Código: {rc}")
    client.subscribe(TOPIC_SUBSCRIBE)

def on_message(client, userdata, msg):
    try:
        payload_str = msg.payload.decode('utf-8')
        print(f"\n[RECEBIDO] Tópico: {msg.topic} | Payload: {payload_str}")
        
        # Tenta converter para JSON
        payload_json = json.loads(payload_str)
        
        # Achata o JSON (ex: tensao_a: {value: 220} vira tensao_a_value: 220)
        flat_data = flatten_json(payload_json)
        
        # Adiciona info do tópico e client_id (se viesse no payload, mas aqui adicionamos manual)
        flat_data['mqtt_topic'] = msg.topic
        
        save_to_csv(flat_data)
        
    except json.JSONDecodeError:
        print(f"[ERRO] Payload não é um JSON válido: {msg.payload}")
    except Exception as e:
        print(f"[ERRO GERAL] {e}")

# --- Loop Principal ---


def main():
    client = mqtt.Client()
    client.on_connect = on_connect
    client.on_message = on_message

    print(f"Tentando conectar em {BROKER_ADDRESS}:{BROKER_PORT}...")
    
    while True:
        try:
            client.connect(BROKER_ADDRESS, BROKER_PORT, 60)
            client.loop_forever()
        except Exception as e:
            print(f"Falha na conexão: {e}. Tentando novamente em 5s...")
            time.sleep(5)

if __name__ == "__main__":
    main()