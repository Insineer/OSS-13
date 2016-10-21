#pragma once

#include <list>
#include <string>
#include <vector>

#include <SFML/Graphics.hpp>

using std::list;
using std::string;
using std::vector;

using sf::IntRect;

struct TextureSpriteInfo{
	int firstFrame;
	int frames;
	bool directed;

	TextureSpriteInfo() { firstFrame = -1; frames = 0; directed = false; }
};

class Texture {
private:
	sf::Texture *texture;
	int key;
	int sizeOfTile;
	int xNumOfTiles, yNumOfTiles;
	IntRect rect;
	vector<TextureSpriteInfo> spritesInfo;

public:
	Texture(int key, string path, int sizeOfTile, list<Texture *> textures, int numOfSprites) : key(key), sizeOfTile(sizeOfTile), spritesInfo(numOfSprites) {
		texture = new sf::Texture();
		texture->loadFromFile(path);

		xNumOfTiles = texture->getSize().x / sizeOfTile;
		yNumOfTiles = texture->getSize().y / sizeOfTile;

		textures.push_back(this);
	}

	void SetInfo(int num, int firstFrame, bool directed, int frames) {
		spritesInfo[num].firstFrame = firstFrame;
		spritesInfo[num].directed = directed;
		spritesInfo[num].frames = frames;
	}

	bool PixelTransparent(int x, int y, int sprite, int direction, int frame) {
		int realState = spritesInfo[sprite].firstFrame;
		if (spritesInfo[sprite].directed > 0) realState += direction * spritesInfo[sprite].frames;
		if (spritesInfo[sprite].frames > 1) realState += frame;

		x += realState % xNumOfTiles * sizeOfTile;
		y += realState / xNumOfTiles * sizeOfTile;

		if (x < 0 || x >= texture->getSize().x || y < 0 || y >= texture->getSize().y) return true;
		if (texture->copyToImage().getPixel(x, y).a == 0) return true;

		return false;
	}

	int GetKey() { return key; }

	sf::Texture *GetSFMLTexture() { return texture; }

	sf::IntRect GetSpriteRect(int sprite, int direction, int frame) {
		int realState = spritesInfo[sprite].firstFrame;
		if (spritesInfo[sprite].directed > 0) realState += direction * spritesInfo[sprite].frames;
		if (spritesInfo[sprite].frames > 1) realState += frame;

		rect.left = realState % xNumOfTiles * sizeOfTile;
		rect.top = realState / xNumOfTiles * sizeOfTile;
		rect.width = rect.height = sizeOfTile;
		return rect;
	}

	int GetSizeOfTile() { return sizeOfTile; }

	~Texture() { if (texture) delete texture; }
};

class Sprite
{
private:
	Texture *texture;
	int num;
	sf::Sprite *sprite;
	float scale;
	int animated;
	int directed;

public:
	Sprite() {
		texture = nullptr;
		num = 0;
		directed = animated = -1;
		sprite = new sf::Sprite();
	}

	Sprite(Texture *t, int num, int direction, int frame) {
		sprite = new sf::Sprite();
		SetTexture(t);
		SetSpriteState(num, direction, frame);
	}

	void SetTexture(Texture *t) {
		texture = t;
		sprite->setTexture(*texture->GetSFMLTexture());
		scale = 1;
	}

	void SetSpriteState(int num, int direction, int frame) {
		this->num = num;
		animated = frame;
		directed = direction;
		sprite->setTextureRect(texture->GetSpriteRect(num, directed, animated));
	}

	void SetSize(int size) {
		scale = size / texture->GetSizeOfTile();
		sprite->setScale(sf::Vector2f(scale, scale));
	}

	void Draw(sf::RenderWindow *window, int x, int y)
	{
		sprite->setPosition(x, y);
		window->draw(*sprite);
	}

	bool PixelTransparent(int x, int y)
	{
		x /= scale; y /= scale;
		if (texture->PixelTransparent(x, y, num, directed, animated)) return true;
		return false;
	}

	~Sprite()
	{
		if (sprite) delete sprite;
	}
};