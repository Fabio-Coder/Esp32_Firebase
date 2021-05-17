# from firebase import firebase
import datetime
import threading
from time import sleep

import firebase_admin
from google.cloud import firestore

default_app = firebase_admin.initialize_app()


# firebase = firebase.FirebaseApplication('https://esp32-autocom-default-rtdb.firebaseio.com', None)


def quickstart_new_instance():
    # [START quickstart_new_instance]
    # [START firestore_setup_client_create]

    # Project ID is determined by the GCLOUD_PROJECT environment variable
    db = firestore.Client()
    # [END firestore_setup_client_create]
    # [END quickstart_new_instance]

    return db


def quickstart_get_collection():
    db = firestore.Client()
    # [START quickstart_get_collection]
    users_ref = db.collection(u'users')
    docs = users_ref.stream()

    for doc in docs:
        print(f'{doc.id} => {doc.to_dict()}')
    # [END quickstart_get_collection]


def quickstart_add_data_one():
    db = firestore.Client()
    # [START quickstart_add_data_one]
    # [START firestore_setup_dataset_pt1]
    doc_ref = db.collection(u'users').document(u'alovelace')
    doc_ref.set({
        u'first': u'Ada',
        u'last': u'Lovelace',
        u'born': 1815
    })
    # [END firestore_setup_dataset_pt1]
    # [END quickstart_add_data_one]


def quickstart_add_data_two():
    db = firestore.Client()
    # [START quickstart_add_data_two]
    # [START firestore_setup_dataset_pt2]
    doc_ref = db.collection(u'users').document(u'aturing')
    doc_ref.set({
        u'first': u'Alan',
        u'middle': u'Mathison',
        u'last': u'Turing',
        u'born': 1912
    })
    # [END firestore_setup_dataset_pt2]
    # [END quickstart_add_data_two]


def conecta_e_exibe():
    contador = firebase.get('/Status', 'Contador')
    maquina = firebase.get('/Status', 'IdentificacaoMaquina')
    status_maquina = firebase.get('/Status', 'Status')
    print('=============================')
    print(f'=========  {maquina}  =========')
    print(f'Contador de pecas: {contador}')
    print(f'Status atual da máquina: {status_maquina}')


if __name__ == '__main__':

    quickstart_add_data_one()
    quickstart_add_data_two()
    quickstart_get_collection()

    for _ in range(10):
        # conecta_e_exibe()
        sleep(10)
