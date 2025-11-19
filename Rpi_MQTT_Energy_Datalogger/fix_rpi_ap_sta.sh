#!/usr/bin/env bash
# =============================================================================
# fix_rpi_ap_sta.sh
#
# Ajusta AP+STA em Raspberry Pi Zero 2W (Ubuntu Server 22.04)
# - MANTÉM a configuração atual de wlan0 (STA) como está
# - Cria interface uap0 (AP) em cima da wlan0
# - Configura hostapd + dnsmasq (apenas DHCP, sem DNS)
# - Garante Mosquitto acessível em 0.0.0.0:1883
# =============================================================================

set -e

# ========================
# CONFIGURAÇÕES EDITÁVEIS
# ========================

# Wi-Fi do AP da RPi (para ESP32 e afins)
AP_SSID="MiEnergy_AP"
AP_PASS="0123456789"

# Rede do AP (uap0)
AP_IP="192.168.50.1"
AP_NETMASK="255.255.255.0"
AP_DHCP_RANGE_START="192.168.50.10"
AP_DHCP_RANGE_END="192.168.50.200"
AP_DHCP_LEASE_TIME="24h"

COUNTRY_CODE="BR"
WIFI_CHANNEL="6"

# =============================================================================

check_root() {
  if [ "$(id -u)" -ne 0 ]; then
    echo "[ERRO] Este script deve ser executado como root (sudo)."
    exit 1
  fi
}

install_packages() {
  echo "==> Instalando hostapd, dnsmasq e mosquitto (se necessário)..."
  apt update
  apt install -y hostapd dnsmasq mosquitto mosquitto-clients iw net-tools
}

configure_mosquitto_listener() {
  echo "==> Garantindo que Mosquitto escute em 0.0.0.0:1883..."
  mkdir -p /etc/mosquitto/conf.d

  cat >/etc/mosquitto/conf.d/01-listener.conf <<EOF
listener 1883 0.0.0.0
allow_anonymous true
EOF

  systemctl restart mosquitto
  systemctl enable mosquitto
  echo "   -> Verificando porta 1883:"
  ss -tulpn | grep 1883 || true
}

create_uap0_service() {
  echo "==> Criando/atualizando serviço systemd create_uap0..."

  cat >/etc/systemd/system/create_uap0.service <<EOF
[Unit]
Description=Create virtual AP interface uap0
After=network-pre.target
Wants=network-pre.target
Before=hostapd.service

[Service]
Type=oneshot
ExecStart=/sbin/iw dev wlan0 interface add uap0 type __ap
ExecStart=/sbin/ip link set uap0 up
ExecStart=/sbin/ip addr add ${AP_IP}/24 dev uap0
RemainAfterExit=yes

[Install]
WantedBy=multi-user.target
EOF

  systemctl daemon-reload
  systemctl enable create_uap0.service
  systemctl start create_uap0.service || true
}

configure_hostapd() {
  echo "==> Configurando hostapd..."

  cat >/etc/hostapd/hostapd.conf <<EOF
interface=uap0
driver=nl80211
ssid=${AP_SSID}
hw_mode=g
channel=${WIFI_CHANNEL}
country_code=${COUNTRY_CODE}
ieee80211n=1
wmm_enabled=1
auth_algs=1
wpa=2
wpa_key_mgmt=WPA-PSK
rsn_pairwise=CCMP
wpa_passphrase=${AP_PASS}
EOF

  # Desmascarar e apontar para o arquivo de config
  systemctl unmask hostapd || true

  if grep -q '^#DAEMON_CONF=' /etc/default/hostapd 2>/dev/null; then
    sed -i 's|^#DAEMON_CONF=.*|DAEMON_CONF="/etc/hostapd/hostapd.conf"|' /etc/default/hostapd
  elif grep -q '^DAEMON_CONF=' /etc/default/hostapd 2>/dev/null; then
    sed -i 's|^DAEMON_CONF=.*|DAEMON_CONF="/etc/hostapd/hostapd.conf"|' /etc/default/hostapd
  else
    echo 'DAEMON_CONF="/etc/hostapd/hostapd.conf"' >> /etc/default/hostapd
  fi

  systemctl enable hostapd
}

configure_dnsmasq() {
  echo "==> Configurando dnsmasq para DHCP somente em uap0..."

  # Backup do dnsmasq.conf principal, se ainda não existir
  if [ ! -f /etc/dnsmasq.conf.orig ]; then
    cp /etc/dnsmasq.conf /etc/dnsmasq.conf.orig
  fi

  # Assegura que dnsmasq use /etc/dnsmasq.d e não faça DHCP/DNS global
  cat >/etc/dnsmasq.conf <<EOF
conf-dir=/etc/dnsmasq.d,.conf
EOF

  mkdir -p /etc/dnsmasq.d

  cat >/etc/dnsmasq.d/ap.conf <<EOF
# Desabilita DNS (para não disputar a porta 53 com systemd-resolved)
port=0

interface=uap0
bind-interfaces

dhcp-range=${AP_DHCP_RANGE_START},${AP_DHCP_RANGE_END},${AP_NETMASK},${AP_DHCP_LEASE_TIME}
dhcp-option=3,${AP_IP}
# DNS para os clientes: pode ser o do roteador ou um público (1.1.1.1)
dhcp-option=6,1.1.1.1
EOF

  systemctl enable dnsmasq
}

restart_services() {
  echo "==> Reiniciando serviços create_uap0, hostapd e dnsmasq..."

  systemctl restart create_uap0.service || true
  systemctl restart hostapd || true
  systemctl restart dnsmasq || true

  echo "==> Status dos serviços:"
  systemctl status create_uap0.service --no-pager || true
  systemctl status hostapd.service --no-pager || true
  systemctl status dnsmasq.service --no-pager || true
}

show_summary() {
  echo "============================================================"
  echo "AP+STA (tentativa) configurado."
  echo
  echo "STA (wlan0):"
  echo "  - Permanece como já estava (netplan original)."
  echo "  - Continue acessando a RPi via IP que já usa (ex.: 192.168.68.105)."
  echo
  echo "AP (uap0):"
  echo "  - SSID: ${AP_SSID}"
  echo "  - Senha: ${AP_PASS}"
  echo "  - IP da RPi: ${AP_IP}"
  echo "  - Faixa DHCP: ${AP_DHCP_RANGE_START} - ${AP_DHCP_RANGE_END}"
  echo
  echo "Mosquitto:"
  echo "  - Escutando em 0.0.0.0:1883"
  echo "  - Teste na RPi:   mosquitto_sub -h localhost -t \"#\" -v"
  echo "  - Teste no PC:    mosquitto_sub -h 192.168.68.105 -t \"#\" -v"
  echo "  - ESP32 no AP:    broker = ${AP_IP}, porta 1883"
  echo "============================================================"
}

main() {
  check_root
  install_packages
  configure_mosquitto_listener
  create_uap0_service
  configure_hostapd
  configure_dnsmasq
  restart_services
  show_summary
}

main
