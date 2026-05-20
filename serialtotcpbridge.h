#ifndef SERIALTOTCPBRIDGE_H
#define SERIALTOTCPBRIDGE_H

#include <QTcpServer>
#include <QTcpSocket>
#include <QSerialPort>
#include <QList>

#include "structs.h"

class SerialToTcpBridge : public QObject
{
    Q_OBJECT

public:
    SerialToTcpBridge(int maxConCount = 10, quint16 tcpPort = 12345, QObject *parent = nullptr) : QObject(parent){
        m_tcpServer = new QTcpServer(this);
        // Настройка TCP сервера
        connect(m_tcpServer, &QTcpServer::newConnection, this, &SerialToTcpBridge::onNewConnection);
        isReadyWrite = 0;
        this->maxConCount = maxConCount;
        this->tcpPort = tcpPort;
    }

    ~SerialToTcpBridge() {
        stop();
        // Очищаем всех клиентов
        for (QTcpSocket* client : m_clients) {
            client->disconnectFromHost();
            client->deleteLater();
        }
        m_clients.clear();
    }

    // Инициализация и запуск сервера
    bool start() {
        if (!m_tcpServer->listen(QHostAddress::Any, tcpPort)) {
            qDebug() << "Не удалось запустить TCP сервер на порту" << tcpPort;
            return false;
        }
        isRunning = 1;
        return true;
    }

    void stop(){
        m_tcpServer->close();
    }

    void write(QByteArray data){
        buff = data;
        if (buff.isEmpty()) return;
        // Транслируем данные ВСЕМ подключенным клиентам
        // Используем итератор вручную, чтобы безопасно удалять сломанные соединения
        for (auto it = m_clients.begin(); it != m_clients.end(); ) {
            QTcpSocket *client = *it;
            qint64 bytesWritten = client->write(buff);
            if (bytesWritten == -1) {
                // Если запись не удалась (клиент разорвал соединение)
                client->deleteLater();
                it = m_clients.erase(it);
                emit newConnectionCount();
            } else {
                ++it;
            }
        }
        buff.clear();
    }

    int getMaxConCount(){
        return maxConCount;
    }

    int getConCount(){
        return m_clients.count();
    }

private slots:
    // Новый клиент подключился к TCP серверу
    void onNewConnection() {
        if (maxConCount == m_clients.count()){
            qDebug() << "Превышен лимит клиентов";
            return;
        }
        QTcpSocket *clientSocket = m_tcpServer->nextPendingConnection();
        if (clientSocket) {
            // Добавляем клиента в список
            m_clients.append(clientSocket);
            qDebug() << "Новый клиент подключен. Всего клиентов:" << m_clients.size();

            // --- Управление отключением клиента ---
            // Слот для удаления клиента из списка при его отключении
            auto handleDisconnect = [this, clientSocket]() {
                m_clients.removeOne(clientSocket);
                clientSocket->deleteLater();
                qDebug() << "Клиент отключен. Осталось:" << m_clients.size();
                emit newConnectionCount();
            };
            connect(clientSocket, &QTcpSocket::disconnected, this, handleDisconnect);
            connect(clientSocket, &QTcpSocket::disconnected, clientSocket, &QTcpSocket::deleteLater);
            emit newConnectionCount();
        }
    }

signals:
    void newConnectionCount();

private:
    bool isReadyWrite;
    bool isRunning;
    int maxConCount;
    quint16 tcpPort;
    QTcpServer* m_tcpServer;
    QByteArray buff;
    QList<QTcpSocket*> m_clients; // Список активных клиентов
};

#endif // SERIALTOTCPBRIDGE_H
