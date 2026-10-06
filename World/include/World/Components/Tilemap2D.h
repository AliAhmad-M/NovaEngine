#pragma once
#include <Core/Utilities/Vector2.h>
#include <Core/ECS/ECS.h>
#include <Core/Services.h>
#include <Core/Debug.h>
#include <Graphics/Texture.h>
#include <nlohmann/json.hpp>
#include <fstream>
#include <sstream>
#include <cmath>
#include <vector>
#include <climits>
#include <algorithm>

namespace Nova 
{
	struct Tile2D
	{
		const Texture* tileset = nullptr;
		float tx = 0.0f, ty = 0.0f, tw = 0.0f, th = 0.0f;
	};

	struct TilemapLayer2D
	{
		std::string name;
		std::vector<std::vector<int>> grid;
	};

	struct TilemapRegion2D
	{
		size_t startRow, startCol;
		size_t endRow, endCol;
	};

	struct TilesetSource2D
	{
		std::string texturePath;
		int firstgid = -1;
	};

	enum class TilemapChildType2D
	{
		LayerSprite,
		RectCollider
	};

	struct Tilemap2D
	{
		int collisionLayer = 0;
		std::string collisionGids;		// comma-separated, e.g. "1,2,5"
		std::string pendingTexturePath;	// Path for tileset to be addded
		int pendingFirstgid = -1;

		bool NeedsRebuild() const { return m_dirty; }
		void MarkRebuildNeeded() { m_dirty = true; }
		void ClearRebuildFlag() { m_dirty = false; }

		Tilemap2D(int tileW = 16, int tileH = 16)
		{
			m_tileW = std::max(1, tileW);
			m_tileH = std::max(1, tileH);
		}

		~Tilemap2D() 
		{
			ECS* ecs = Services::Fetch<ECS>();
			for (const auto& [childEntity, type] : m_childEntities)
			{
				if (ecs->EntityExists(childEntity)) 
					ecs->DeleteEntity(childEntity);
			}

			m_childEntities.clear();
		}

		void AddTileset(const Texture* tileset, int firstgid = -1)
		{
			addTilesetInternal(tileset, firstgid);
		}

		void AddTileset(const Texture* tileset, const std::string& texturePath, int firstgid = -1)
		{
			int resolvedFirstgid = addTilesetInternal(tileset, firstgid);
			m_tilesetSources.push_back({ texturePath, resolvedFirstgid });
		}

		void LoadTilemap(const std::string& mapPath)
		{
			Debug::LogInfo("Loading tilemap file: %s", mapPath.c_str());
			std::ifstream mapfile(mapPath);

			if (!mapfile.is_open())
			{
				Debug::LogError("Failed to open tilemap");
				return;
			}

			nlohmann::json mapfilejson;
			try
			{
				mapfilejson = nlohmann::json::parse(mapfile);
			}
			catch (const nlohmann::json::parse_error& e)
			{
				Debug::LogWarn("Unable to load tilemap: %s", e.what());
				return;
			}

			bool isInfinite = mapfilejson.value("infinite", false);

			if (isInfinite)
			{
				Debug::LogInfo("Processing infinite tilemap");
				loadInfiniteTilemap(mapfilejson);
			}
			else
			{
				Debug::LogInfo("Processing finite tilemap");
				loadFiniteTilemap(mapfilejson);
			}

			m_sourceMapPath = mapPath;

			Debug::LogInfo("Tilemap '%s' loaded successfully", mapPath.c_str());
		}

		void GenerateCollisionData(int layer, const std::vector<int>& tiles)
		{
			layer = std::max(0, layer);

			if (layer >= (int)m_layers.size() || m_mapH == 0 || m_mapW == 0)
			{
				Debug::LogError("Failed to generate tilemap collision data");
				return;
			}

			// Collision flags
			std::vector<std::vector<bool>> collision(m_mapH, std::vector<bool>(m_mapW, false));

			for (int row = 0; row < m_mapH; row++)
			{
				for (int col = 0; col < m_mapW; col++)
				{
					int tileIndex = m_layers[layer].grid[row][col];
					collision[row][col] = std::find(tiles.begin(), tiles.end(), tileIndex) != tiles.end();
				}
			}

			for (size_t row = 0; row < (size_t)m_mapH; row++)
			{
				size_t col = 0;
				while (col < (size_t)m_mapW)
				{
					if (collision[row][col])
					{
						size_t startCol = col;
						while (col < (size_t)m_mapW && collision[row][col])
						{
							col++;
						}
						size_t endCol = col;

						size_t endRow = row;
						bool isCollidable = true;

						while (isCollidable && endRow + 1 < (size_t)m_mapH)
						{
							for (size_t currentCol = startCol; currentCol < endCol; currentCol++)
							{
								if (!collision[endRow + 1][currentCol])
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

						// Flag processed cells as resolved
						for (size_t r = row; r <= endRow; r++) {
							for (size_t c = startCol; c < endCol; c++) {
								collision[r][c] = false;
							}
						}

						m_collisionRegions.push_back({ row, startCol, endRow + 1, endCol });
					}
					else
					{
						col++;
					}
				}
			}
		}

		int GetMapWidth() const { return m_mapW; }
		int GetMapHeight() const { return m_mapH; }
		int GetTileWidth() const { return m_tileW; }
		int GetTileHeight() const { return m_tileH; }
		const Vector2& GetOffset() const { return m_offset; }

		const std::vector<Tile2D>& GetTileset() const
		{
			return m_tileset;
		}

		const std::vector<TilemapLayer2D>& GetLayers() const
		{
			return m_layers;
		}

		const std::vector<TilemapRegion2D>& GetCollisionRegions() const
		{
			return m_collisionRegions;
		}

		void AddChildEntity(EntityID entity, TilemapChildType2D type)
		{
			m_childEntities.push_back({ entity, type });
		}

		const std::vector<std::pair<EntityID, TilemapChildType2D>>& GetChildEntities() const
		{
			return m_childEntities;
		}

		const std::string& GetSourceMapPath() const { return m_sourceMapPath; }
		void SetSourceMapPath(const std::string& path) { m_sourceMapPath = path; }

		const std::vector<TilesetSource2D>& GetTilesetSources() const { return m_tilesetSources; }

		void AddTilesetSourceRecord(const std::string& texturePath, int firstgid)
		{
			m_tilesetSources.push_back({ texturePath, firstgid });
		}

		void ReflectFields(Inspectables& visitor)
		{
			char buf[64];

			std::snprintf(buf, sizeof(buf), "%d x %d", m_mapW, m_mapH);
			visitor.ReadOnly("Map size", buf);

			std::snprintf(buf, sizeof(buf), "%zu regions", m_collisionRegions.size());
			visitor.ReadOnly("Collision", buf);

			std::snprintf(buf, sizeof(buf), "%zu entities", m_childEntities.size());
			visitor.ReadOnly("Children", buf);

			for (size_t i = 0; i < m_layers.size(); i++)
			{
				std::snprintf(buf, sizeof(buf), "Layer %zu", i);
				visitor.ReadOnly(buf, m_layers[i].name.empty() ? "(unnamed)" : m_layers[i].name.c_str());
			}

			visitor.Header("Map");

			visitor.Path("Path", &m_sourceMapPath, ".json"); // picker changes are marked dirty by the inspector

			int oldW = m_tileW, oldH = m_tileH;
			visitor.Int("Tile W", &m_tileW);
			visitor.Int("Tile H", &m_tileH);
			m_tileW = std::max(1, m_tileW);
			m_tileH = std::max(1, m_tileH);
			if (m_tileW != oldW || m_tileH != oldH) m_dirty = true;

			visitor.Group("Tilesets", [&]
				{
					int removeIdx = -1;

					for (int i = 0; i < (int)m_tilesetSources.size(); i++)
					{
						std::string n = std::to_string(i);
						TilesetSource2D& src = m_tilesetSources[i];
						std::string oldTex = src.texturePath;
						int oldGid = src.firstgid;

						visitor.Path(("Texture " + n).c_str(), &src.texturePath, ".png");
						visitor.Int(("First GID " + n).c_str(), &src.firstgid);
						visitor.Button(("Remove##" + n).c_str(), [&removeIdx, i] { removeIdx = i; });

						if (src.texturePath != oldTex || src.firstgid != oldGid) m_dirty = true;
					}

					if (removeIdx >= 0)
					{
						m_tilesetSources.erase(m_tilesetSources.begin() + removeIdx);
						m_dirty = true;
					}

					visitor.Path("New Texture", &pendingTexturePath, ".png");
					visitor.Int("New First GID", &pendingFirstgid);
					visitor.ReadOnly("", "First GID: -1 = auto");
					visitor.Button("Add Tileset", [this]
						{
							if (pendingTexturePath.empty()) return;
							m_tilesetSources.push_back({ pendingTexturePath, pendingFirstgid });
							pendingTexturePath.clear();
							pendingFirstgid = -1;
							m_dirty = true;
						});
				});

			visitor.Header("Collision");

			int oldLayer = collisionLayer;
			std::string oldGids = collisionGids;
			visitor.Int("Layer", &collisionLayer);
			visitor.String("GIDs", &collisionGids);
			visitor.ReadOnly("", "Comma-separated e.g. 1,2,5");
			collisionLayer = std::max(0, collisionLayer);
			if (collisionLayer != oldLayer || collisionGids != oldGids) m_dirty = true;
		}


	private:
		int addTilesetInternal(const Texture* tileset, int firstgid)
		{
			Debug::LogInfo("Loading tileset");
			if (!tileset)
			{
				Debug::LogError("Tileset texture cannot be NULL");
				return firstgid;
			}

			int width = tileset->GetWidth();
			int height = tileset->GetHeight();

			int rows = height / m_tileH;
			int cols = width / m_tileW;

			// If no firstgid provided, auto‑calculate (continuous after previous tileset)
			if (firstgid < 0)
				firstgid = (int)m_tileset.size() + 1;

			int totalTiles = rows * cols;
			int requiredSize = firstgid + totalTiles - 1;

			if ((int)m_tileset.size() < requiredSize)
			{
				m_tileset.resize(requiredSize, Tile2D());
			}

			for (int i = 0; i < rows; i++)
			{
				for (int j = 0; j < cols; j++)
				{
					size_t tileIndex = firstgid - 1 + (i * cols + j);

					m_tileset[tileIndex] = {
						tileset,
						(float)(j * m_tileW) / width,
						(float)(i * m_tileH) / height,
						(float)m_tileW / width,
						(float)m_tileH / height
					};
				}
			}

			Debug::LogInfo("Tileset loaded");
			return firstgid;
		}

		void initTextureMap(int layers, int rows, int cols)
		{
			layers = std::max(1, layers);
			rows = std::max(1, rows);
			cols = std::max(1, cols);

			m_layers.resize(layers);
			for (size_t layer = 0; layer < (size_t)layers; layer++)
			{
				m_layers[layer].grid.resize(rows);
				for (size_t row = 0; row < (size_t)rows; row++)
				{
					m_layers[layer].grid[row].resize(cols, -1);
				}
			}
		}

		void loadFiniteTilemap(const nlohmann::json& mapfilejson)
		{
			m_mapW = mapfilejson["width"];
			m_mapH = mapfilejson["height"];

			initTextureMap((int)mapfilejson["layers"].size(), m_mapH, m_mapW);

			for (size_t i = 0; i < mapfilejson["layers"].size(); i++)
			{
				auto layout = mapfilejson["layers"][i]["data"];
				if (mapfilejson["layers"][i].contains("name"))
					m_layers[i].name = mapfilejson["layers"][i]["name"];

				for (size_t j = 0; j < layout.size(); j++)
				{
					size_t row = j / m_mapW;
					size_t col = j % m_mapW;

					int rawGid = layout[j];
					m_layers[i].grid[row][col] = rawGid - 1;
				}
			}
		}

		void loadInfiniteTilemap(const nlohmann::json& mapfilejson)
		{
			int minX = INT_MAX, minY = INT_MAX, maxX = INT_MIN, maxY = INT_MIN;

			for (size_t layerIdx = 0; layerIdx < mapfilejson["layers"].size(); layerIdx++)
			{
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

			m_mapW = maxX - minX;
			m_mapH = maxY - minY;
			m_offset.x = (float)minX;
			m_offset.y = (float)minY;

			initTextureMap((int)mapfilejson["layers"].size(), m_mapH, m_mapW);

			for (size_t layerIdx = 0; layerIdx < mapfilejson["layers"].size(); layerIdx++)
			{
				const auto& layer = mapfilejson["layers"][layerIdx];
				if (layer.contains("name"))
					m_layers[layerIdx].name = layer["name"];

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

						int globalCol = chunkX + localCol - (int)m_offset.x;
						int globalRow = chunkY + localRow - (int)m_offset.y;

						if (globalCol < 0 || globalRow < 0 || globalCol >= m_mapW || globalRow >= m_mapH)
						{
							continue;
						}

						int tileId = chunkData[chunkTileIdx];
						m_layers[layerIdx].grid[globalRow][globalCol] = tileId - 1;
					}
				}
			}
		}

		bool m_dirty = false;

		int m_mapW = 0;
		int m_mapH = 0;

		int m_tileW;
		int m_tileH;

		Vector2 m_offset = Vector2::ZERO;

		std::vector<Tile2D>  m_tileset;
		std::vector<TilemapLayer2D> m_layers;
		std::vector<TilemapRegion2D> m_collisionRegions;

		std::vector<std::pair<EntityID, TilemapChildType2D>> m_childEntities;

		std::string m_sourceMapPath;
		std::vector<TilesetSource2D> m_tilesetSources;
	};
}