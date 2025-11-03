#!/bin/bash
# DayDream BBS Installer für Debian mit systemd
# Erstellt systemd-Services statt xinetd

set -e

if [ "$(whoami)" != "root" ]; then
  echo "Bitte als root ausführen!"
  exit 1
fi

# Farben für Ausgabe
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

echo -e "${GREEN}=== DayDream BBS Installer für Debian mit systemd ===${NC}"
echo ""

# Abhängigkeiten installieren
echo -e "${YELLOW}Installiere Abhängigkeiten...${NC}"
export DEBIAN_FRONTEND=noninteractive

apt-get update
apt-get -y upgrade

apt-get install -y build-essential \
    python-setuptools python-dev python2.7-dev python-software-properties \
    libpq-dev \
    libtiff4-dev libjpeg8-dev zlib1g-dev libfreetype6-dev liblcms2-dev \
    libwebp-dev tcl8.5-dev tk8.5-dev \
    git curl

# Optional: CVS, SVN, Mercurial für Entwicklung
echo ""
echo "Sollen zusätzliche Entwicklungstools (cvs, svn, mercurial) installiert werden? (j/N)"
read INSTALL_DEV_TOOLS
if [[ "$INSTALL_DEV_TOOLS" =~ ^[JjYy] ]]; then
    apt-get install -y cvs subversion mercurial
fi

# Prüfen ob wir im DayDream Repository sind oder von GitHub klonen sollen
SOURCE_DIR=""
if [ -d "SRC" ] && [ -f "SRC/make.sh" ]; then
    # Wir sind bereits im DayDream Repository
    SOURCE_DIR=$(pwd)
    echo -e "${GREEN}DayDream Quellcode bereits vorhanden, verwende: $SOURCE_DIR${NC}"
elif [ -d "/tmp/daydream" ] && [ -f "/tmp/daydream/SRC/make.sh" ]; then
    # Bereits geklont in /tmp
    SOURCE_DIR="/tmp/daydream"
    echo -e "${GREEN}DayDream Quellcode bereits geklont, verwende: $SOURCE_DIR${NC}"
else
    echo ""
    echo "DayDream Quellcode nicht gefunden. Soll von GitHub geklont werden? (J/n)"
    read CLONE_FROM_GITHUB
    if [[ ! "$CLONE_FROM_GITHUB" =~ ^[Nn] ]]; then
        echo -e "${YELLOW}Klone DayDream von GitHub...${NC}"
        cd /tmp
        if [ -d "daydream" ]; then
            rm -rf daydream
        fi
        git clone https://github.com/ryanfantus/daydream.git
        SOURCE_DIR="/tmp/daydream"
        cd $SOURCE_DIR
        echo -e "${GREEN}Klonen abgeschlossen${NC}"
    else
        echo "Bitte DayDream Quellcode manuell bereitstellen oder das Script im Repository-Verzeichnis ausführen."
        exit 1
    fi
fi

# Installationspfad abfragen
echo ""
echo "Bitte Installationspfad eingeben (Standard: /home/bbs)"
read INSTALL_PATH
INSTALL_PATH=${INSTALL_PATH:-/home/bbs}

# Pfad bereinigen (trailing slash entfernen)
INSTALL_PATH=$(echo "$INSTALL_PATH" | sed 's/\/$//')

# Benutzer-Abfrage
echo ""
echo "Existieren die BBS-Benutzer (bbs, bbsadmin, zipcheck) bereits? (J/n)"
read INPUT
if [[ "$INPUT" =~ ^[Nn] ]]; then
    ADD_USERS=1
else
    ADD_USERS=0
fi

if [ $ADD_USERS -eq 1 ]; then
    echo ""
    echo -e "${YELLOW}Erstelle BBS-Benutzer in 5 Sekunden...${NC}"
    sleep 5
    
    # zipcheck Gruppe und Benutzer
    if ! getent group zipcheck > /dev/null 2>&1; then
        groupadd zipcheck
    fi
    if ! id zipcheck > /dev/null 2>&1; then
        useradd -g zipcheck -s /bin/false zipcheck
    fi
    
    # bbs Gruppe und Benutzer
    if ! getent group bbs > /dev/null 2>&1; then
        groupadd bbs
    fi
    if ! id bbsadmin > /dev/null 2>&1; then
        useradd -d $INSTALL_PATH -m -G zipcheck -g bbs bbsadmin
        echo "Bitte Passwort für bbsadmin eingeben:"
        passwd bbsadmin
    fi
    if ! id bbs > /dev/null 2>&1; then
        useradd -d $INSTALL_PATH -G zipcheck -g bbs -s /bin/false bbs
        echo "Bitte Passwort für bbs eingeben:"
        passwd bbs
    fi
    
    chown bbsadmin:bbs $INSTALL_PATH
    chmod 750 $INSTALL_PATH
fi

# Daten-Dateien installieren
echo ""
echo "Sollen die Daten-Dateien installiert werden? (Nur bei neuen Installationen) (J/n)"
read INPUT
if [[ "$INPUT" =~ ^[Nn] ]]; then
    INSTALL_DATA=0
else
    INSTALL_DATA=1
fi

# Wechsel ins Source-Verzeichnis
cd "$SOURCE_DIR"

# Dokumentation kopieren
echo -e "${YELLOW}Kopiere Dokumentation...${NC}"
cd DOCS
if [ ! -d $INSTALL_PATH/docs ]; then
    mkdir -p $INSTALL_PATH/docs
fi
cp -R * $INSTALL_PATH/docs
cd ..

# Daten-Dateien installieren (falls noch nicht gemacht)
if [ $INSTALL_DATA -eq 1 ]; then
    echo -e "${YELLOW}Installiere Daten-Dateien...${NC}"
    cd INSTALL
    bash install_me.sh $INSTALL_PATH
    cd ..
fi

# Quellcode kompilieren
echo -e "${YELLOW}Kompiliere Quellcode...${NC}"
cd SRC
bash make.sh build
INSTALL_PATH=$INSTALL_PATH bash make.sh install
OLD_PWD=$(pwd)
cd $INSTALL_PATH
. scripts/ddenv.sh
ddcfg configs/daydream.cfg
cd $OLD_PWD
bash secure.sh $INSTALL_PATH

# systemd-Services erstellen
echo -e "${YELLOW}Erstelle systemd-Services...${NC}"

# ownbbs Script erstellen
cat > $INSTALL_PATH/ownbbs <<EOF
#!/bin/bash
chown -R bbs.bbs $INSTALL_PATH
chmod 775 $INSTALL_PATH
chown zipcheck $INSTALL_PATH/utils/runas
chmod u+s $INSTALL_PATH/utils/runas
EOF
chmod +x $INSTALL_PATH/ownbbs

# rundd Script erstellen
cat > $INSTALL_PATH/rundd <<EOF
#!/bin/bash
$INSTALL_PATH/ownbbs
$INSTALL_PATH/bin/ddtelnetd -ubbs
EOF
chmod +x $INSTALL_PATH/rundd

# systemd Socket für Telnet (Port 23)
cat > /etc/systemd/system/daydream-telnet.socket <<EOF
[Unit]
Description=DayDream BBS Telnet Socket
Before=daydream-telnet@.service

[Socket]
ListenStream=23
Accept=yes
EOF

# systemd Service für Telnet
cat > /etc/systemd/system/daydream-telnet@.service <<EOF
[Unit]
Description=DayDream BBS Telnet Service
After=network.target daydream-telnet.socket

[Service]
Type=simple
ExecStart=$INSTALL_PATH/rundd
StandardInput=socket
StandardOutput=socket
StandardError=journal
User=root
EOF

# Optional: FTP Service (wenn ddftpd vorhanden ist)
if [ -f $INSTALL_PATH/bin/ddftpd ]; then
    cat > /etc/systemd/system/daydream-ftp.socket <<EOF
[Unit]
Description=DayDream BBS FTP Socket
Before=daydream-ftp@.service

[Socket]
ListenStream=21
Accept=yes
EOF

    cat > /etc/systemd/system/daydream-ftp@.service <<EOF
[Unit]
Description=DayDream BBS FTP Service
After=network.target daydream-ftp.socket

[Service]
Type=simple
ExecStart=$INSTALL_PATH/scripts/runftp.sh
StandardInput=socket
StandardOutput=socket
StandardError=journal
User=root
Environment=DAYDREAM=$INSTALL_PATH
EOF
fi

# systemd neu laden und Services aktivieren
systemctl daemon-reload
systemctl enable daydream-telnet.socket
systemctl start daydream-telnet.socket

if [ -f /etc/systemd/system/daydream-ftp.socket ]; then
    systemctl enable daydream-ftp.socket
    systemctl start daydream-ftp.socket
fi

# Shell für bbs Benutzer setzen (falls neu erstellt)
if [ $ADD_USERS -eq 1 ]; then
    usermod -s $INSTALL_PATH/scripts/ddlogin.sh bbs
fi

# Alte for_inetd.conf entfernen falls vorhanden
if [ -f $INSTALL_PATH/for_inetd.conf ]; then
    rm $INSTALL_PATH/for_inetd.conf
fi

echo ""
echo -e "${GREEN}=== Installation abgeschlossen! ===${NC}"
echo ""
echo "DayDream BBS wurde installiert in: $INSTALL_PATH"
echo ""
echo "Systemd-Services wurden erstellt:"
echo "  - daydream-telnet.socket (Port 23)"
if [ -f /etc/systemd/system/daydream-ftp.socket ]; then
    echo "  - daydream-ftp.socket (Port 21)"
fi
echo ""
echo "Status prüfen mit:"
echo "  systemctl status daydream-telnet.socket"
echo ""
echo "Testen mit:"
echo "  telnet localhost"
echo ""
echo -e "${GREEN}Viel Erfolg mit DayDream BBS!${NC}"
