#include <Runtime/Tilemap/Tilemap.h>
#include <Core/Utilities/Matrix4x4.h>
#include <Core/Components.h>
#include <Physics/Components.h>
#include <Runtime/ContentLoader.h>
#include <Graphics/Components.h>
#include <cmath>

Tilemap::Tilemap(ECS* ecs, int tileSizeX, int tileSizeY, const Vector2& scale)
	: m_ecs(ecs)
{
	m_tileSizeX = std::max(1, tileSizeX);
	m_tileSizeY = std::max(1, tileSizeY);

	m_scale.x = std::max(0.001f, scale.x);
	m_scale.y = std::max(0.001f, scale.y);
}

void Tilemap::AddTileset(const Texture* tileset, int firstgid) 
{
	printf("[INFO]: Loading tileset\n");
	if (!tileset) 
	{
		fprintf(stderr, "[ERROR]: Tileset texture cannot be NULL\n");
		return;
	}

	int width  = tileset->GetWidth();
	int height = tileset->GetHeight();

	int rows = height / m_tileSizeY;
	int cols = width / m_tileSizeX;
	
	// Calculate total no. of tiles
	int totalTiles   = rows * cols;
	int requiredSize = firstgid + totalTiles - 1; // Space required to accomodate new tileset
	
	// Allocate space for new tiles
	if (m_tiles.size() < requiredSize) 
	{
		m_tiles.resize(requiredSize, Tile());
	}

	// Populate the tile array with tile data
	for (int i = 0; i < rows; i++)
	{
		for (int j = 0; j < cols; j++)
		{
			size_t tileIndex = firstgid - 1 + (i * cols + j);

			m_tiles[tileIndex] = {
				tileset,
				(float)(j * m_tileSizeX) / width,
				(float)(i * m_tileSizeY) / height,
				(float)m_tileSizeX / width,
				(float)m_tileSizeY / height
			};
		}
	}
	printf("[INFO]: Tileset loaded\n");

}

void Tilemap::initTextureMap(int layers, int rows, int cols) 
{
	layers = std::max(1, layers);
	rows   = std::max(1, rows);
	cols   = std::max(1, cols);

	m_layers.resize(layers);
	for (size_t layer = 0; layer < layers; layer++)
	{
		m_layers[layer].grid.resize(rows);
		for (size_t row = 0; row < rows; row++)
		{
			m_layers[layer].grid[row].resize(cols, -1); // -1 means empty tile
		}
	}
}

void Tilemap::LoadTilemap(const std::string& path) 
{
	printf("[INFO]: Loading tilemap file: %s\n", path.c_str());
	std::ifstream mapfile(path);

	// Open JSON file
	if (!mapfile.is_open())
	{
		fprintf(stderr, "[ERROR]: Failed to open tilemap\n");
		return;
	}

	// Parse JSON file
	nlohmann::json mapfilejson;
	try
	{
		mapfilejson = nlohmann::json::parse(mapfile);
	}
	catch (const nlohmann::json::parse_error& e)
	{
		fprintf(stderr, "[ERROR]: Unable to load tilemap - %s\n", e.what());
		return;
	}

	// Check if tilemap has infinite flag or not
	bool isInfinite = mapfilejson.value("infinite", false);

	if (isInfinite)
	{
		printf("[INFO]: Processing infinite tilemap\n");
		loadInfiniteTilemap(mapfilejson);
	}
	else
	{
		printf("[INFO]: Processing finite tilemap\n");
		loadFiniteTilemap(mapfilejson);
	}

	printf("[INFO]: Tilemap '%s' loaded successfully\n", path.c_str());
}

void Tilemap::loadFiniteTilemap(const nlohmann::json& mapfilejson) 
{
	// Get tilemap map properties
	m_mapWidth  = mapfilejson["width"];
	m_mapHeight = mapfilejson["height"];
	
	// Initialize the texture map
	initTextureMap((int)mapfilejson["layers"].size(), m_mapHeight, m_mapWidth);

	// Load all of the layer data
	for (size_t i = 0; i < mapfilejson["layers"].size(); i++)
	{
		auto layout = mapfilejson["layers"][i]["data"];

		for (size_t j = 0; j < layout.size(); j++)
		{
			size_t row = j / m_mapWidth;
			size_t col = j % m_mapWidth;

			int rawGid = layout[j];

			// Store the raw array layout index
			m_layers[i].grid[row][col] = rawGid - 1;
		}
	}
}

void Tilemap::loadInfiniteTilemap(const nlohmann::json& mapfilejson) 
{
	int minX = INT_MAX, minY = INT_MAX, maxX = INT_MIN, maxY = INT_MIN;

	for (size_t layerIdx = 0; layerIdx < mapfilejson["layers"].size(); layerIdx++)
	{
		// Load map properties in chunks for each layer
		const auto& layer = mapfilejson["layers"][layerIdx];
		if (layer.contains("chunks") && !layer["chunks"].empty())
		{
			for (const auto& chunk : layer["chunks"])
			{
				int chunkX = chunk["x"];
				int chunkY = chunk["y"];
				int chunkWidth = chunk["width"];
				int chunkHeight = chunk["height"];

				minX = std::min(minX, chunkX);
				minY = std::min(minY, chunkY);
				maxX = std::max(maxX, chunkX + chunkWidth);
				maxY = std::max(maxY, chunkY + chunkHeight);
			}
		}
	}

	if (minX == INT_MAX)
	{
		minX = minY = 0;
		maxX = maxY = 16;
	}

	m_mapWidth = maxX - minX;
	m_mapHeight = maxY - minY;
	m_offsetX = minX;
	m_offsetY = minY;

	// Initialize the texture map
	initTextureMap((int)mapfilejson["layers"].size(), m_mapHeight, m_mapWidth);

	for (size_t layerIdx = 0; layerIdx < mapfilejson["layers"].size(); layerIdx++)
	{
		// Load map data in chunks for each layer
		const auto& layer = mapfilejson["layers"][layerIdx];
		if (!layer.contains("chunks") || layer["chunks"].empty()) continue;

		for (const auto& chunk : layer["chunks"])
		{
			int chunkX = chunk["x"];
			int chunkY = chunk["y"];
			int chunkWidth = chunk["width"];
			int chunkHeight = chunk["height"];
			auto chunkData = chunk["data"];

			for (int chunkTileIdx = 0; chunkTileIdx < (int)chunkData.size(); chunkTileIdx++)
			{
				int localCol = chunkTileIdx % chunkWidth;
				int localRow = chunkTileIdx / chunkWidth;

				int globalCol = chunkX + localCol - m_offsetX;
				int globalRow = chunkY + localRow - m_offsetY;

				if (globalCol < 0 || globalRow < 0 || globalCol >= (int)m_mapWidth || globalRow >= (int)m_mapHeight)
				{
					continue;
				}

				int tileId = chunkData[chunkTileIdx];
				m_layers[layerIdx].grid[globalRow][globalCol] = tileId - 1;
			}
		}
	}
}

void Tilemap::GenerateMap(Renderer* renderer, const Vector2& origin, float rotation, int startZIndex)
{
	if (m_layers.empty() || m_tiles.empty()) return;

	int pixelW = (int)(m_mapWidth * m_tileSizeX);
	int pixelH = (int)(m_mapHeight * m_tileSizeY);

	// Calculate the offset needed to start drawing at the top-left of a symmetric Orthographic projection
	float orthoOffsetX = -(pixelW / 2.0f);
	float orthoOffsetY = -(pixelH / 2.0f);

	for (size_t layerIdx = 0; layerIdx < m_layers.size(); layerIdx++)
	{
		const Layer& layer = m_layers[layerIdx];

		Texture* layerTexture = new Texture();
		if (!layerTexture->CreateRenderTexture(pixelW, pixelH))
		{
			fprintf(stderr, "[ERROR]: Failed to create render texture for layer %zu\n", layerIdx);
			delete layerTexture;
			continue;
		}

		layerTexture->BindFBO();
		layerTexture->ClearFBO();

		renderer->Begin(Matrix4x4::CreateOrthographic((float)pixelW, (float)pixelH, -1, 1));

		for (size_t row = 0; row < layer.grid.size(); row++)
		{
			for (size_t col = 0; col < layer.grid[row].size(); col++)
			{
				int tileIndex = layer.grid[row][col];
				if (tileIndex < 0 || tileIndex >= (int)m_tiles.size()) continue;

				const Tile& tile = m_tiles[tileIndex];
				if (!tile.tileset) continue;

				// Apply the projection offset so (0,0) maps to the top-left of the FBO
				float x = orthoOffsetX + ((float)col * m_tileSizeX);
				float y = orthoOffsetY + ((float)row * m_tileSizeY);
				float w = (float)m_tileSizeX;
				float h = (float)m_tileSizeY;

				renderer->DrawQuad(
					tile.tileset->GetHandle(),
					{ x, y }, { w, h },
					0.0f,
					tile.tx, tile.ty, tile.tw, tile.th
				);
			}
		}

		renderer->Flush();
		layerTexture->UnbindFBO();

		EntityID entity = m_ecs->CreateEntity();
		m_ecs->AddComponent(entity, Transform2D(origin, rotation, m_scale));
		m_ecs->AddComponent(entity, Sprite2D(layerTexture, 1, 1, startZIndex + (int)layerIdx));
	}
}

void Tilemap::initCollisionMap()
{
	m_collision.resize(m_mapHeight);
	for (size_t row = 0; row < m_mapHeight; row++)
	{
		m_collision[row].resize(m_mapWidth, false);
	}
}

void Tilemap::AddCollision(int layer, const std::vector<int>& tiles)
{
	layer = std::max(0, layer);

	initCollisionMap();
	printf("[INFO]: Generating collision map\n");
	for (size_t row = 0; row < m_mapHeight; row++)
	{
		for (size_t col = 0; col < m_mapWidth; col++)
		{
			int tileIndex = m_layers[layer].grid[row][col];
			// Match against our layer array indexes
			m_collision[row][col] = std::find(tiles.begin(), tiles.end(), tileIndex) != tiles.end();
		}
	}

	generateCollisionTiles();
	m_collision.clear();
	printf("[INFO]: Collision map generated\n");
}

void Tilemap::generateCollisionTiles()
{
	for (size_t row = 0; row < m_mapHeight; row++)
	{
		size_t col = 0;
		while (col < m_mapWidth)
		{
			// Group consecutive collision tiles into single collider
			if (m_collision[row][col])
			{
				size_t startCol = col;
				while (col < m_mapWidth && m_collision[row][col])
				{
					col++;
				}
				size_t endCol = col;

				size_t endRow = row;
				bool isCollidable = true;

				while (isCollidable && endRow + 1 < m_mapHeight)
				{
					for (size_t currentCol = startCol; currentCol < endCol; currentCol++)
					{
						if (!m_collision[endRow + 1][currentCol])
						{
							isCollidable = false;
							break;
						}
					}
					if (isCollidable)
					{
						endRow++;
					}
				}

				// Flag processed layout cells as resolved so we don't duplicate shapes
				for (size_t r = row; r <= endRow; r++) {
					for (size_t c = startCol; c < endCol; c++) {
						m_collision[r][c] = false;
					}
				}

				float w = (float)(endCol - startCol) * m_tileSizeX;
				float h = (float)(endRow - row + 1) * m_tileSizeY;

				float x = (float)startCol * m_tileSizeX * m_scale.x;
				float y = (float)row * m_tileSizeY * m_scale.y;

				// Create tile entity
				EntityID tileColliderEntity = m_ecs->CreateEntity();
				m_ecs->AddComponent(tileColliderEntity, Transform2D(Vector2(x, y), 0.0f, m_scale));
				m_ecs->AddComponent(tileColliderEntity, Collider2D(Vector2(w / 2.0f, h / 2.0f), w, h, true, true));
			}
			else
			{
				col++;
			}
		}
	}
}