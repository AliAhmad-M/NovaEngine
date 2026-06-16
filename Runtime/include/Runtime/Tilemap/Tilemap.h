#pragma once

#include <vector>
#include <fstream>
#include <sstream>
#include <nlohmann/json.hpp>
#include <iostream>
#include <Core/EngineContext.h>
#include <Core/Utilities/Vector2.h>
#include <Graphics/Texture.h>

struct Tile 
{
	const Texture* tileset = nullptr;
	float tx = 0.0f, ty = 0.0f, tw = 0.0f, th = 0.0f;
};

struct Layer 
{
	std::string name;
	std::vector<std::vector<int>> grid;
};

class Tilemap
{
public:
	Tilemap() = default;
	Tilemap(EngineContext* ctx, int tileSizeX = 16, int tileSizeY = 16, const Vector2& scale = Vector2::ONE);

	void AddTileset(const Texture* tileset, int firstgid);
	void LoadTilemap(const std::string& path);
	void GenerateMap(
		const Vector2& origin = Vector2::ZERO,
		float rotation = 0.0f, int startZIndex = 0
	);
	void AddCollision(int layer, const std::vector<int>& tiles);

private:
	EngineContext* m_ctx;

	int m_mapWidth = 0;
	int m_mapHeight = 0;

	int m_tileSizeX;
	int m_tileSizeY;

	Vector2 m_scale;

	int m_offsetX = 0;
	int m_offsetY = 0;

	// Helper function to load finite Tiled tilemap
	void loadFiniteTilemap(const nlohmann::json& mapfilejson);

	// Helper function to load infinite Tiled tilemap
	void loadInfiniteTilemap(const nlohmann::json& mapfilejson);

	// Array of tiles
	std::vector<Tile> m_tiles;

	// Array of layers
	std::vector<Layer> m_layers;

	// Texture map, stores index of textures in tilemap
	std::vector<std::vector<std::vector<int>>> m_texture;

	// Collision map, stores if a tile has collision or not
	std::vector<std::vector<bool>> m_collision;

	void initTextureMap(int layers, int rows, int cols);
	void initCollisionMap();
	void generateCollisionTiles();
};