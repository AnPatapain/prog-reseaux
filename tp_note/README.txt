Pour compiler: "make"
Pour clean: "make clean"

1) Fonctionalités
    > client normal et client registré
    > changer le nickname pour le client normal et changer le nickname avec mot de passe
    > Envoyer le message privé
    > Registrer et Unregistrer
    > exit
    > alerte broadcast et alerte privé
    > afficher date du serveur
    > envoyer le fichier

2) La logique de envoyer le fichier
    Il y a trois acteurs dans ce cas utilisation: server, client qui veut envoyer le fichier et
    le recipient qui va recevoir le fichier

    > le client ouvrir le fichier avec "r" permission
    > si le fichier est bien ouvert le client envoyer le signal is_open_ok au serveur. Et le client
    commence a envoyer au serveur des meta data (file name, size)
    > si is_open_ok = True. Le serveur recevoir les meta datas
    > le serveur trouve le recipient dans 2 liste (client_liste et regis_client_liste)
    > le serveur informe le recipient que le message va être le fichier pas message normal
    > le serveur envoyer le meta data a recipient
    > le recipient créer le fichier et envoyer le signal ready_to_receive_file au serveur
    > le serveur envoyer le signal ready_to_receive_file a la client
    > la client lire le fichier et envoyer au serveur
    > le serveur distribue le fichier au recipient


