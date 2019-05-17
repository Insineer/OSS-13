#include "Player.hpp"

#include <memory>

#include <Server.hpp>
#include <Network/Connection.hpp>
#include <World/World.hpp>
#include <World/Tile.hpp>
#include <World/Objects.hpp>
#include <World/Map.hpp>
#include <ClientUI/WelcomeWindowSink.h>

#include <Shared/Command.hpp>

class Server;

Player::Player(std::string ckey) : ckey(ckey) {
    game = nullptr;
	control = nullptr;
}

void Player::SetConnection(sptr<Connection> &connection) {
	this->connection = connection;
	if (camera && control) {
		camera->SetPosition(control->GetOwner()->GetTile());
	}
}

void Player::UpdateServerList() {
	if (sptr<Connection> connect = connection.lock())
		connect->commandsToClient.Push(new GameListServerCommand());
}

void Player::JoinToGame(Game *game) {
	this->game = game;
	actions.Push(new JoinPlayerCommand);
}

void Player::ChatMessage(std::string &message) {
    game->GetChat()->AddMessage("<" + ckey + ">" + message);
}

void Player::Move(uf::Direction direction) {
	actions.Push(new MovePlayerCommand(uf::DirectionToVect(direction)));
}

void Player::MoveZ(bool up) {
	actions.Push(new MoveZPlayerCommand(up));
}

void Player::ClickObject(uint id) {
    actions.Push(new ClickObjectPlayerCommand(id));
}

void Player::Drop() {
	actions.Push(new DropPlayerCommand());
}

void Player::Build() {
	actions.Push(new BuildPlayerCommand());
}

void Player::Ghost() {
	actions.Push(new GhostPlayerCommand());
}

void Player::UIInput(const std::string &handle, std::unique_ptr<network::protocol::UIData> &&data) {
	if (uiSinks.size())
		uiSinks[0]->OnInput(handle, std::forward<std::unique_ptr<network::protocol::UIData>>(data));
}

void Player::CallVerb(const std::string &verb) {
	auto delimiter = verb.find(".");
	std::string verbHolder = verb.substr(0, delimiter);
	std::string verbName = verb.substr(delimiter + 1);

	auto verbHolderIter = verbsHolders.find(verbHolder);

	if (verbHolderIter != verbsHolders.end()) {
		auto v = verbHolderIter->second->GetVerbs();
		auto pair = v.find(verbName);
		auto f = pair->second;
		f(this);
	}
	else
		Server::log << "Bad VerbHolder: " << verbHolder << std::endl;
}

void Player::Update() {
    while (!actions.Empty()) {
        PlayerCommand *temp = actions.Pop();
        if (temp) {
            switch (temp->GetCode()) {
                case PlayerCommand::Code::JOIN: {
                    SetControl(game->GetStartControl(this));
					verbsHolders["atmos"] = GetControl()->GetOwner()->GetTile()->GetMap()->GetAtmos();
					OpenWindow<WelcomeWindowSink>();
                    break;
                }
				case PlayerCommand::Code::MOVE: {
					if (control) {
						MovePlayerCommand *moveCommand = dynamic_cast<MovePlayerCommand *>(temp);
						control->MoveCommand(moveCommand->order);
					}
					break;
				}
				case PlayerCommand::Code::MOVEZ: {
					if (control) {
						MoveZPlayerCommand *moveZCommand = dynamic_cast<MoveZPlayerCommand *>(temp);
						control->MoveZCommand(moveZCommand->order);
					}
					break;
				}
                case PlayerCommand::Code::CLICK_OBJECT: {
                    if (control) {
                        auto clickObjectPlayerCommand = dynamic_cast<ClickObjectPlayerCommand *>(temp);
                        control->ClickObjectCommand(clickObjectPlayerCommand->id);
                    }
                    break;
                }
				case PlayerCommand::Code::DROP: {
					if (!control) break;
					if (auto *creature = dynamic_cast<Creature *>(control->GetOwner()))
						creature->Drop();
					break;
				}
				case PlayerCommand::Code::BUILD: {
					if (!control) break;
					Tile *tile = control->GetOwner()->GetTile();
					if (tile)
						CurThreadGame->GetWorld()->CreateObject<Wall>(tile);
					break;
				}
				case PlayerCommand::Code::GHOST: {
					if (!control) break;
					auto *ghost = dynamic_cast<::Ghost *>(control->GetOwner());
					if (!ghost) {
						ghost = CurThreadGame->GetWorld()->CreateObject<::Ghost>(control->GetOwner()->GetTile());
						ghost->SetHostControl(control);
						SetControl(ghost->GetComponent<Control>());
					} else {
						SetControl(ghost->GetHostControl());
                        ghost->Delete();
					}
					break;
				}
                default:
                    break;
            }
            delete temp;
        }
    }

    if (control->GetOwner()->GetTile() != camera->GetPosition()) camera->SetPosition(control->GetOwner()->GetTile());
}

void Player::SendGraphicsUpdates(sf::Time timeElapsed) {
    if (camera) {
        camera->UpdateView(timeElapsed);
    }
}

void Player::Suspend() {
	camera->Suspend();
}

void Player::SetControl(Control *control) {
	if (this->control) this->control->player = nullptr;
    this->control = control;
	control->player = this;
    SetCamera(new Camera(control->GetOwner()->GetTile()));
    camera->SetPlayer(this);
	// Get Ability to see Invisibile from the mob (if control owner is mob)
	if (Creature *creature = dynamic_cast<Creature *>(control->GetOwner()))
		camera->SetInvisibleVisibility(creature->GetInvisibleVisibility());
};

void Player::SetCamera(Camera *camera) { this->camera.reset(camera); }

sptr<Connection> Player::GetConnection() {
	return connection.lock();
}

Control *Player::GetControl() { return control; }
Camera *Player::GetCamera() { return camera.get(); }
bool Player::IsConnected() { return !connection.expired(); }

void Player::AddCommandToClient(ServerCommand *command) {
	if (sptr<Connection> connect = connection.lock())
		connect->commandsToClient.Push(command);
}