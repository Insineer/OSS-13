#include "NetworkController.hpp"

#include <SFML/Network.hpp>

#include <plog/Log.h>

#include <Shared/Global.hpp>
#include <Shared/Network/Archive.h>
#include <Shared/Network/Protocol/InputData.h>
#include <Shared/Network/Protocol/ServerToClient/Commands.h>
#include <Shared/Network/Protocol/ClientToServer/Commands.h>
#include <Shared/ErrorHandling.h>

#include <IServer.h>
#include <Player.hpp>

#include "Connection.hpp"

using namespace network::protocol;

void NetworkController::working() {
    sf::TcpListener listener;
    listener.listen(Global::PORT);

    sf::SocketSelector selector;

    selector.add(listener);

    while (active) {
        if (selector.wait(sf::microseconds(TIMEOUT.count()))) {
            // Receiving from client
            if (selector.isReady(listener)) {
                sf::TcpSocket *socket = new sf::TcpSocket;
                if (listener.accept(*socket) == sf::TcpSocket::Done) {
                    selector.add(*socket);
					Connection *new_connection = new Connection();
					new_connection->socket.reset(socket);
					connections.push_back(sptr<Connection>(new_connection));
                } else
                    LOGE << "New connection accepting error";
            } else {
                for (auto iter = connections.begin(); iter != connections.end();) {
					
                    if (selector.isReady(*iter->get()->socket)) {
                        sf::TcpSocket *socket = iter->get()->socket.get();
                        sptr<Player> player = (*iter)->player;

                        sf::Packet packet;
                        sf::Socket::Status status = socket->receive(packet);
                        switch (status) {
                            case sf::Socket::Done:
								if (!parsePacket(packet, *iter)) {
									selector.remove(*socket);
									iter = connections.erase(iter);
									continue;
								}
                                break;
                            case sf::Socket::Disconnected:
								if (player) {
									LOGI << "Lost client " << player->GetCKey() << " connection";
								} else
									LOGI << "Lost unregistered client signal";
                                selector.remove(*socket);
                                iter = connections.erase(iter);
                                continue;
                                break;
                        }

                    }

                    iter++;
                }
            }
        }

        // Sending to client
        for (auto &connection : connections) {
			while (!connection->commandsToClient.Empty()) {
				sf::Packet packet;
				uf::InputArchive ar(packet);
				network::protocol::Command *command = connection->commandsToClient.Pop();
				EXPECT(command);
				ar << *command;
				delete command;
				connection->socket->send(packet);
			}
        }
    }
}

bool NetworkController::parsePacket(sf::Packet &packet, sptr<Connection> &connection) {
	uf::OutputArchive ar(packet);
	auto p = ar.UnpackSerializable();

	if (auto *command = dynamic_cast<client::AuthorizationCommand *>(p.get())) {
		bool secondConnection = false;
		for (auto &connection : connections) {
			if (connection->player && connection->player->GetCKey() == command->login) {
				secondConnection = true;
				LOGI << "Player " << command->login << " " << command->password << " is trying to authorize second time";
				break;
			}
		}

		if (!secondConnection) {
			if (Player *player = GServer->Authorization(command->login, command->password)) {
				player->SetConnection(connection);
				connection->player = sptr<Player>(player);
				connection->commandsToClient.Push(new network::protocol::server::AuthorizationSuccessCommand());
				return true;
			}
		}
		connection->commandsToClient.Push(new network::protocol::server::AuthorizationFailedCommand());
		return true;
	}

	if (auto *command = dynamic_cast<client::RegistrationCommand *>(p.get())) {
		if (GServer->Registration(command->login, command->password))
			connection->commandsToClient.Push(new network::protocol::server::RegistrationSuccessCommand());
		else
			connection->commandsToClient.Push(new network::protocol::server::RegistrationFailedCommand());
		return true;
	}

	if (auto *command = dynamic_cast<client::JoinGameCommand *>(p.get())) {
		if (connection->player) {
			if (GServer->JoinGame(connection->player)) {
				connection->commandsToClient.Push(new network::protocol::server::GameJoinSuccessCommand());
			} else {
				connection->commandsToClient.Push(new network::protocol::server::GameJoinErrorCommand());
			}
		}
		return true;
	}

	if (auto *command = dynamic_cast<client::MoveCommand *>(p.get())) {
		if (connection->player)
			connection->player->Move(uf::Direction(command->direction));
		return true;
	}

	if (auto *command = dynamic_cast<client::MoveZCommand *>(p.get())) {
		if (connection->player)
			connection->player->MoveZ(command->up);
		return true;
	}

	if (auto *command = dynamic_cast<client::ClickObjectCommand *>(p.get())) {
		if (connection->player)
			connection->player->ClickObject(command->id);
		return true;
	}

	if (auto *command = dynamic_cast<client::SendChatMessageCommand *>(p.get())) {
		if (connection->player)
			connection->player->ChatMessage(command->message);
		return true;
	}

	if (auto *command = dynamic_cast<client::DisconnectionCommand *>(p.get())) {
		if (connection->player)
			LOGI << "Client " << connection->player->GetCKey() << " disconnected";
		return false;
	}

	if (auto *command = dynamic_cast<client::UIInputCommand *>(p.get())) {
		if (connection->player) {
			connection->player->UIInput(std::move(command->data));
		}
		return true;
	}

	if (auto *command = dynamic_cast<client::UITriggerCommand *>(p.get())) {
		if (connection->player) {
			connection->player->UITrigger(command->window, command->trigger);
		}
		return true;
	}

	if (auto *command = dynamic_cast<client::CallVerbCommand *>(p.get())) {
		if (connection->player) {
			connection->player->CallVerb(command->verb);
		}
		return true;
	}

	if (connection->player)
		LOGE << "Unknown Command is received from " << connection->player->GetCKey();
	else
		LOGE << "Unknown Command is received from unregistered client"; 

	return true;
}

NetworkController::NetworkController() {
    active = false;
}

void NetworkController::Start() {
    if (active) return;
    active = true;
    thread.reset(new std::thread(&NetworkController::working, this));
}

void NetworkController::Stop() {
    if (!active) return;
    active = false;
    thread->join();
}