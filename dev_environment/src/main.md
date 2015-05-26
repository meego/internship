---
title: Installation d'un environnement de développement pour le noyau ALMOS-MK
author: Pierre-Yves PÉNEAU
creator: Pierre-Yves PÉNEAU
tags: almos-mk tsar soclib systemc systemcass development
date: 18 Mai 2015
language: frenchb

abstract: Cette documentation s'adresse aux personnes voulant installer un
  environnement de développement pour le noyau ALMOS-MK
  (Multi-Kernel). Néanmoins, les instructions données ici peuvent facilement
  s'adapter pour la version monolithique 32 bits d'ALMOS. On considère que le
  lecteur dispose d'un compte sur le dépôt Bitbucket d'ALMOS et sur le réseau
  recherche. Dans le cas contraire, prendre contact avec
  [Manuel Bouyer](mailto:manuel.bouyer@lip6.fr). Lors du téléchargement des
  sources sur les dépôts SVN, c'est le mot de passe *Web* qui est demandé.

---

# Introduction

L'environnement de développement est composé de différents couches
logicielles. Il est important de bien comprendre quelle est leur utilité et les
liens entre elles. On trouve:

* les bibliothèques SystemC
* le simulateur SystemCASS
* Soclib (TODO)
* l'architecture matérielle TSAR
* le noyau ALMOS-MK

SystemC est un émulateur de composants matériels. Il est l'ancêtre de
SystemCASS. Néanmoins, SystemCASS a besoin de certaineo bibliothèques offertes par
SystemC, c'est pourquoi nous devons installer les deux. Soclib nous permet
d'avoir un TTy et un FrameBuffer (TODO) Enfin, TSAR est ici le modèle Soclib de
l'architecture matérielle qui tournera sur le simulateur SystemCASS.

Pour une meilleur organisation, il est conseillé d'installer tous ces composants
dans un répertoire de travail. Tout au long de ce document, nous y ferons
référence avec la variable d'environnemenet `$ROOT_DIR`.

    export ROOT_DIR=~/workspace
    mkdir -p $ROOT_DIR

Pour des raisons d'efficacité, il est préférable de compiler tous les outils
avec une version récente de `gcc`. La version actuellement par défaut sur les
serveurs de calcul est `gcc-4.4.7`. La version `4.9.0`
compilée par Clément Devigne est conseillée. La compilation des outils par une
version récente de gcc apporte des optimisations non négligeables permettant
d'accélérer la vitesse du simulateur. Pour l'utiliser, il faut simplement
redéfinir les variables suivantes:

    export PATH=~devigne/gcc/bin
    export LD_LIBRARY_PATH=~devigne/gcc/lib64


# SystemC

Les sources de SystemC sont disponibles sur le
[site officiel](http://www.accellera.org/). Il en existe également une copie sur
le réseau recherche de l'équipe ALSOC (`~systemc/systemc-2.1.v1/`). Néanmoins,
cette version est ancienne. L'exemple ici est donné avec la version 2.3.1 de
SystemC téléchargée depuis le site officiel.

Extraire l'archive dans le dossier `$ROOT_DIR` et renommer le dossier extrait:

    cd $ROOT_DIR
    tar -xf /path/to/systemc-2.3.1.tgz
    mv systemc-2.3.1 systemc

Préparer un dossier temporaire et compiler :

	mkdir -p systemc/objdir
	cd systemc/objdir
	CXXFLAGS=-O3 ../configure --prefix=$ROOT_DIR/systemc
	make -j 4
	make install

Pour finir, définissez la variable d'environnement `$SYSTEMC` en la faisant
pointer sur le répertoire `systemc` :

    export SYSTEMC=$ROOT_DIR/systemc


# SystemCASS

Les sources sont disponibles sur le dépôt SVN de Soclib.

    cd $ROOT_DIR
    svn co --username <login> https://www.soclib.fr/svn/systemcass/sources systemcass

On prépare les dossiers nécessaires pour une compilation et une installation
locale:

    cd systemcass
    ./bootstrap
    mkdir -p objdir usr
    cd objdir
    ../configure --prefix=$ROOT_DIR/systemcass/usr

On compile :

    make -j 4
	make install

On oublie pas de définir la variable `$SYSTEMCASS`

    export SYSTEMCASS=$ROOT_DIR/systemcass


# Soclib

Les sources sont disponibles sur le dépôt SVN du projet :

    cd $ROOT_DIR
    svn co https://www.soclib.fr/svn/trunk/soclib soclib
    cd soclib/utils/src

On étend la variable `$PATH` pour la compilation. On a besoin du
binaire `soclib-cc` présent dans le répertoire `soclib/utils/bin`.

    PATH=$ROOT_DIR/soclib/utils/bin:$PATH make -j 4
	make install

On définit la variable d'environnement `$SOCLIB`

    export SOCLIB=$ROOT_DIR/soclib


# Le GIET_VM

Le GIET ne sera pas utilisé ici. Nous allons uniquement profiter de l'outil
`genmap` présenté ci-après.

    cd $ROOT_DIR
    svn co https://www-soc.lip6.fr/svn/giet-vm/soft/giet_vm giet-vm
	export GIET_TOP=ROOT_DIR/giet-vm


# ALMOS-MK

La compilation du noyau est semblable à celle du noyau Linux. Les sources sont
disponible depuis le dépot Git du projet:

    git clone git@bitbucket.org:almos_merge/almosmerge almos

Ensuite, il faut changer de branche pour `almos-mk`

    cd almos
    git fetch && git checkout almos-mk

Il ne reste plus qu'à lancer la compilation

    make

That's it, le noyau est prêt !


# TSAR

Les sources sont sur le dépôt SVN du projet.

    cd $ROOT_DIR
    svn co https://www-soc.lip6.fr/svn/tsar/trunk tsar
	export TSAR_TOP=$ROOT_DIR/tsar

Lorsque l'on évoque l'architecture TSAR, il y a deux composants: le pré-loader
et la plateforme en elle-même.

Nous considérons ici l'architecture de base **tsar_generic_leti**. C'est cette
architecture qui est actuellement en construction au CEA-Leti.

## hard_config.h

  Dans un premier temps, il faut générer le header qui défini l'architecture
  matérielle. Ce header est utilisé pour la construction du pré-loader et de
  l'architecture elle-même. L'outil utilisé pour générer ce fichier est
  `genmap`, que nous avons brièvement présenté précédemment. Ce script python va
  générer un fichier `hard_config.h` avec les caractéristiques principales de
  l'architecture. Il est nécessaire de lui fournir quelques informations.

* Pour quelle plateforme nous souhaitons générer le fichier :

    `--arch=$TSAR_TOP/platforms/tsar_generic_leti`

* Où générer le fichier :

    `--hard=$TSAR_TOP/platforms/tsar_generic_leti`

* Le nombre de processeurs par clusters (2 par défaut)

    `--p=4`

* Le nombre de terminaux que nous allons utiliser (il n'y en a qu'un seul par
  défaut) :

    `--tty=3 # 3 terminaux en plus de celui de base`

Il ne reste plus qu'à appeler le script avec ces options :

    python $GIET_TOP/soft/giet_vm/giet_python/genmap \
                    --arch=$TSAR_TOP/platforms/tsar_generic_leti \
                    --hard=$TSAR_TOP/platforms/tsar_generic_leti \
					--p=4 \
					--tty=3

Si cette opération est effectuée régulièrement, on peut définir la variable
`$TSAR_PLATFORM` :

    export $TSAR_PLATFORM=$TSAR_TOP/platforms/tsar_generic_leti


**Remarque:** Le script effectue une phase de pré-compilation. Il va donc créer un
fichier `arch.pyc`. Vous pouvez le supprimer une fois le fichier `hard_config.h`
créé. Idem dans le dossier de `giet_python`.

## Le pré-loader

Le pré-loader est le code qui sera mis dans la ROM de la plateforme et qui a
pour but d'aller charger les données présentent dans le secteur numéro 3 du
disque dur. À ce secteur se situe le **bootloader** d'ALMOS-MK, à ne justement
pas confondre avec le **pré-loader**. Le bootloader aura pour rôle de charger le
système.

Le pré-loader est situé dans le dossier `tsar/softs/tsar_boot`. Par défaut, une
version est fournie lors de la récupération des source. Néanmoins, pour le
reconstruire, il faut générer le fichier `hard_config.h` qui contient la
configuration matérielle de la plateforme. Ensuite, on compile en donnant au
Makefile le chemin vers le fichier `hard_config.h`

    cd $ROOT_DIR/tsar/softs/tsar_boot
    make HARD_CONFIG_PATH=$TSAR_TOP/platforms/tsar_generic/leti

## La plateforme Leti

Il ne reste plus qu'à compiler la représentation de l'architecture. Pour cela,
on va dans le répertoire `$TSAR_TOP/platforms/tsar_generic_leti`, puis on
appelle la commande `make`. Une fois compilée, il faut indiquer au binaire le
chemin vers le disque dur de la plateforme de cette manière:

    ./simul.x -DISK $ALMOS_TOP/hdd-img.bin

Et voilà, la simulation se lance comme par magie ! Pour utiliser des fichiers à
la place des TTy Soclib, il faut lancer la simulation de cette manière:

    SOCLIB_TTY=FILES ./simul.x

Pour se passer du FrameBuffer, on utilise SOCLIB_FB:

    SOCLIB_FB=HEADLESS ./simul.

Ces variables peuvent évidemment être utlisées ensembles:
	
    SOCLIB_TTY=FILES SOCLIB_FB=HEADLESS ./simul.x

# Conclusion

Enjoy !
