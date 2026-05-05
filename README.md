# DSBM (Disseny de Sistemes Basats en Microcontroladors)


## Informació General

Les sessions de laboratori d'aquest repositori s'han realitzat amb el següent material:
  - PIC24F16KA101
  - Raspberry Pi 2B
  - Arduino UNO
  - TFT Proto rev 1.01 (amb enganxina Touchpanel)


## Estructura del repositori

El repositori està organitzat en directoris segons hem anat fent les entregues.

  - **Entrega 1**: conté els labs 1-4.
  - **Entrega 2**: conté els labs 5-6.
  - **Entrega 3**: conté el lab 7.

Els laboratoris 1-4 eren completament independents; del 5 en endavant, en canvi, cada sessió ampliava el que s’havia fet a l’anterior.
Això implica que el contingut dels directoris dels labs 6+ serà el mateix que el del directori anterior, però amb nous codis afegits.

Per tal d'escriure codi nou sense modificar els ja funcionals de sessions anteriors i tenir-los com a referència, hem anat generant nous codis font per a cada funcionalitat:

#### Raspberry
  - `print_screen.c`: pinta un text, un rectangle i una imatge a la TFT.
  - `uart_basic.c`: imprimeix el que rep del PIC i li envia el que s'entri per teclat.
  - `uart_button.c`: depenent del que enviï el PIC, imprimeix ON o OFF a la TFT.
  - `uart_touchpad.c`: rep coordenades del touchpad des del PIC i les pinta a la TFT.
  - `touchpad_ui.c`: mateix que l'anterior amb botons interactius

