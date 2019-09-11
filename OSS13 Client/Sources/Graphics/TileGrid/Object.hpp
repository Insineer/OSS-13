#pragma once

#include <string>
#include <vector>

#include <Graphics/Sprite.hpp>

#include <Shared/Types.hpp>
#include <Shared/Geometry/Direction.hpp>
#include <Shared/Geometry/DirectionSet.h>
#include <Shared/Global.hpp>
#include <Shared/Network/Protocol/ServerToClient/WorldInfo.h>

class Sprite;
class Tile;

namespace sf {
	class RenderTarget;
	class Time;
	class Packet;
}

#include "iostream"

class Object {
public:
	Object();
    Object(const Object &) = default;
    Object &operator=(Object &) = default;
    ~Object();

	void Draw(sf::RenderTarget *, uf::vec2i windowPos);
	void Update(sf::Time timeElapsed);
    void Resize(uint tileSize);

    void SetID(uint id) { this->id = id; }
	void AddSprite(uint spriteID);
	void ClearSprites();
    void PlayAnimation(uint spriteID);
	void SetDirection(const uf::Direction);
	void SetMoveSpeed(float speed);
	// Second arg is true, if intent was approved by server
	void SetMoveIntent(uf::vec2i moveIntent, bool approved);
	void ResetShiftingState();
	void ReverseShifting(uf::Direction direction);

	uint GetID() const;
	std::string GetName() const;
	//Sprite *GetSprite() const;
	uf::Direction GetDirection() const { return direction; }
	uint GetLayer() const;
    bool PixelTransparent(uf::vec2i pixel) const;

	Tile *GetTile();
	sf::Vector2f GetShift() const;
    sf::Vector2i GetMoveIntent() const;
	//sf::Vector2i GetShiftingDirection() const;
	bool IsDense() const;
	uf::DirectionSet GetSolidity() const;

	friend sf::Packet &operator>>(sf::Packet &packet, Object &object);
	friend std::unique_ptr<Object> CreateObjectWithInfo(const network::protocol::ObjectInfo &objectInfo);
	friend Tile;

private:
    uint id;
    std::string name;
	std::vector<::Sprite> sprites;
    ::Sprite animation;
    bool animationProcess;
    uf::Direction direction;
    uint layer;

	bool density;
	uf::DirectionSet solidity;
	uf::DirectionSetFractional opacity;

    uf::vec2f shift;
    
    float moveSpeed;
    uf::vec2i moveIntent;
    uf::vec2i moveIntentApproved;
    uf::vec2f speed;

	Tile *tile{nullptr};
};