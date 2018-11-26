#! /bin/sh

echo "IRC-Hispano IRCD"
echo ""
echo "Configuracion del IRC Daemon de IRC-Hispano..."
echo ""

PREFIX="none"
BINDIR="none"
SYSCONFDIR="none"
DATADIR="none"

if [ x"$2" = x ]
then
        echo "Uso $0 {PRO|DESA|MANUAL} usuarios [prefijo ruta]"
        echo "  En PRO se instala en /usr/local/ircd/"
        echo "  En DESA se instala en $HOME/bin/ircd"
        echo "  En MANUAL se instala en la ruta especificada"
        exit 1
fi

if [ "$1" = "PRO" ]; then
	PREFIX="/usr/local/ircd"
	BINDIR="/usr/local/ircd"
	SYSCONFDIR="/usr/local/ircd"
	DATADIR="/usr/local/ircd"

elif [ "$1" = "DESA" ]; then
	if [ $USER = "root" ]; then
		PREFIX="/root"
		BINDIR="/bin"
		SYSCONFDIR="/etc/ircd"
		DATADIR="/usr/share/ircd"
	else
		PREFIX="$HOME/bin/ircd"
		BINDIR="$HOME/bin/ircd"
		SYSCONFDIR="$HOME/bin/ircd"
		DATADIR="$HOME/bin/ircd"
	fi

elif [ "$1" = "MANUAL" ]; then
	if [ x"$3" = x ]
	then
		echo "Falta la ruta de instalacion";
		exit 1
	else
		PREFIX=$3
		BINDIR=$3
		SYSCONFDIR=$3
		DATADIR=$3
	fi
else
	echo "Opcion $1 desconocida";
	exit 1
fi

#echo Generando Configure y Makefile
if [ ! -f configure ];
then
	echo Generando Configure y Makefile
	sh autogen.sh
fi

echo Configurando IRCh
./configure --prefix=$PREFIX --bindir=$BINDIR --sysconfdir=$SYSCONFDIR --datadir=$DATADIR \
    --with-maxcon=$2 \
    --enable-pcre \
    --disable-ssl \
    --with-ddb-environment
echo ""
exit 0
