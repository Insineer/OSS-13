#pragma once

#include <Shared/Network/Protocol/Command.h>
#include <Shared/Network/Protocol/ServerToClient/OverlayInfo.h>
#include <Shared/Network/Protocol/ServerToClient/WindowData.h>
#include <Shared/Network/Protocol/ServerToClient/WorldInfo.h>

namespace network {
namespace protocol {
namespace server {

DEFINE_PURE_SERIALIZABLE(AuthorizationSuccessCommand, Command)

DEFINE_PURE_SERIALIZABLE(RegistrationSuccessCommand, Command)

DEFINE_PURE_SERIALIZABLE(AuthorizationFailedCommand, Command)

DEFINE_PURE_SERIALIZABLE(RegistrationFailedCommand, Command)

DEFINE_PURE_SERIALIZABLE(GameCreateSuccessCommand, Command)

DEFINE_PURE_SERIALIZABLE(GameCreateErrorCommand, Command)

DEFINE_PURE_SERIALIZABLE(GameJoinSuccessCommand, Command)

DEFINE_PURE_SERIALIZABLE(GameJoinErrorCommand, Command)

DEFINE_SERIALIZABLE(GraphicsUpdateCommand, Command)
	enum Option {
		EMPTY = 0,
		BLOCKS_SHIFT = 1,
		CAMERA_MOVE = 1 << 1,
		DIFFERENCES = 1 << 2,
		NEW_CONTROLLABLE = 1 << 3
	};

	//std::list<sptr<Diff>> diffs;
	std::vector<TileInfo> tilesInfo;

	Option options;

	uf::vec3i camera;
	uf::vec3i firstTile;
	int controllableId;
	float controllableSpeed;
DEFINE_SERIALIZABLE_END

DEFINE_SERIALIZABLE(OverlayUpdateCommand, Command)
	std::vector<network::protocol::OverlayInfo> overlayInfo;
DEFINE_SERIALIZABLE_END

DEFINE_PURE_SERIALIZABLE(OverlayResetCommand, Command)

DEFINE_SERIALIZABLE(OpenWindowCommand, Command)
	std::string id;
	network::protocol::WindowData data;
DEFINE_SERIALIZABLE_END

DEFINE_SERIALIZABLE(UpdateWindowCommand, Command)
	uptr<network::protocol::UIData> data;
DEFINE_SERIALIZABLE_END

DEFINE_SERIALIZABLE(AddChatMessageCommand, Command)
	std::string message;
DEFINE_SERIALIZABLE_END

} // namespace server
} // namespace protocol
} // namespace network