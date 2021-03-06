#include "Object.hpp"

#include "Shared/Math.hpp"
#include "Shared/Physics/MovePhysics.hpp"

#include <Client.hpp>
#include <Graphics/Window.hpp>
#include <Graphics/UI/UI.hpp>
#include <Graphics/UI/UIModule/GameProcessUI.hpp>
#include <Graphics/Sprite.hpp>
#include <Graphics/TileGrid/TileGrid.hpp>

Object::Object() :
    id(0), 
    animationProcess(false),
    direction(uf::Direction::NONE), 
    layer(0), 
    moveSpeed(0),
    tile(nullptr)
{ }

Object::~Object() {
    if (tile) {
        tile->RemoveObject(this);
    }
}

void Object::Draw(sf::RenderTarget *target, uf::vec2i pos) {
    TileGrid *tileGrid = tile->GetTileGrid();
    if (!tileGrid) {
        std::exception(); // Where is this tile!? 
    }
    uint tileSize = tileGrid->GetTileSize();
    if (animationProcess) {
        animation.Draw(target, pos + shift * tileSize);
    } else {
		for (auto &sprite : sprites) {
			if (sprite.IsValid()) sprite.Draw(target, pos + shift * tileSize);
		}
    }
}

void Object::Update(sf::Time timeElapsed) {
    // Movement

	uf::vec2f deltaShift = uf::phys::countDeltaShift(timeElapsed, shift, moveSpeed, moveIntent, speed);

    shift += deltaShift;

    // Animation and animated icon handling
    if (animationProcess) {
        if (animation.Update(timeElapsed)) {
            animationProcess = false;
        }
    } else {
		for (auto &sprite : sprites) {
			if (sprite.IsValid()) sprite.Update(timeElapsed);
		}
    }
}

void Object::Resize(uint tileSize) {
    if (animationProcess) animation.Resize(tileSize);
	for (auto &sprite : sprites) {
		if (sprite.IsValid()) sprite.Resize(tileSize);
	}
}

void Object::AddSprite(uint id) {
	sprites.push_back(CC::Get()->RM.CreateSprite(id));
    if (sprites.back().IsValid()) 
		sprites.back().SetDirection(direction);
}

void Object::ClearSprites() {
	sprites.clear();
}

void Object::PlayAnimation(uint id) {
    animation = CC::Get()->RM.CreateSprite(id);
    if (animation.IsValid()) {
        animationProcess = true;
        animation.SetDirection(direction);
    }
}

void Object::SetDirection(const uf::Direction newdirection) {
	if (newdirection == uf::Direction::NONE)
		return;

    // cut the diagonal directions
	direction = uf::Direction(char(newdirection) % 4);
	for (auto &sprite : sprites) {
		if (sprite.IsValid()) sprite.SetDirection(direction);
	}
    if (animation.IsValid()) animation.SetDirection(direction);
}

void Object::SetMoveSpeed(float moveSpeed) {
	this->moveSpeed = moveSpeed;
}

void Object::SetMoveIntent(uf::vec2i moveIntent, bool approved) {
    if (approved) {
        this->moveIntentApproved = moveIntent;
        if (uf::abs(this->moveIntent.x) < uf::abs(moveIntentApproved.x))
            this->moveIntent.x = moveIntentApproved.x;
        if (uf::abs(this->moveIntent.y) < uf::abs(moveIntentApproved.y))
            this->moveIntent.y = moveIntentApproved.y;
    } else
	    this->moveIntent = moveIntent;
}

void Object::ResetShiftingState() {
    moveIntent = {};
    shift = {};
}

void Object::ReverseShifting(uf::Direction direction) {
	uf::vec2i directionVect = uf::DirectionToVect(direction);
	if (directionVect.x) moveIntent.x = 0, moveIntentApproved.x = 0;
	if (directionVect.y) moveIntent.y = 0, moveIntentApproved.y = 0;
	shift -= directionVect;
}

uint Object::GetID() const { return id; }
std::string Object::GetName() const { return name; }
//Sprite &Object::GetSprite() const { return sprite; }
uint Object::GetLayer() const { return layer; }
bool Object::PixelTransparent(uf::vec2i pixel) const {
	for (auto &sprite : sprites) {
		if (sprite.IsValid() && !sprite.PixelTransparent(pixel)) 
			return false;
	}
	return true;
}

Tile *Object::GetTile() { return tile; }
sf::Vector2f Object::GetShift() const { return shift; }
sf::Vector2i Object::GetMoveIntent() const { return moveIntent; }
bool Object::IsDense() const { return solidity.IsExistsOne({uf::Direction::CENTER}); }
uf::DirectionSet Object::GetSolidity() const { return solidity; }
