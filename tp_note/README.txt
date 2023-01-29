Pour compiler: "make"
Pour clean: "make clean"

Commentaire: Je pense qu'il serait mieux de catégoriser les messages en différents types : message normal, message d'information, 
message d'erreur, message de fichier. Ensuite, je pourrais changer la couleur en fonction du type de 
message (rouge pour les messages d'erreur, vert pour les messages d'information). 
Cependant, j'ai réalisé cela trop tard (lorsque mon serveur avait déjà plus de 1000 lignes de code). 
Ainsi, j'ai décidé de ne rien changer.

Quand vous testez la fonctionalité envoyer le fichier. Si vous voyez "exit file send-recv flow" dans serveur, client qui envoie le fichier
et client qui recois le fichier c'est a dire la processus send-recv-forward fichier est bien terminé. Si non, on a un bouge.
J'ai fait de mon mieux pour anticiper toutes les erreurs possibles. Cependant, cela peut toujours arriver en marge

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

3) Tester envoyer le fichier
    > Envoyer le text.txt, le fichier text_dst.txt va etre crée automatiquement (par ex: si vous envoyer name.txt le name_dst.txt va etre crée)

4) Mon limite
    > Je ne peut que envoyer le fichier avec la taille < 1024. Parce que j'ai essayé de séparer le fichier en multiples data_chunk et les 
    envoyer en utilisant la boucle While mais ça marche pas.


