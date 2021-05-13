from firebase import firebase
from time import sleep

firebase = firebase.FirebaseApplication('https://esp32-autocom-default-rtdb.firebaseio.com', None)


def conecta_e_exibe():
    contador = firebase.get('/Status', 'Contador')
    maquina = firebase.get('/Status', 'IdentificacaoMaquina')
    status_maquina = firebase.get('/Status', 'Status')
    print('=============================')
    print(f'=========  {maquina}  =========')
    print(f'Contador de pecas: {contador}')
    print(f'Status atual da máquina: {status_maquina}')


if __name__ == '__main__':
    for _ in range(10):
        conecta_e_exibe()
        sleep(10)
