#include <World/Systems/TilemapSystem2D.h>
#include <Core/Services.h>
#include <Core/Debug.h>
#include <Graphics/Components/Sprite2D.h>
#include <Physics/Components/RigidBody2D.h>
#include <World/Components/Tilemap2D.h>
#include <Graphics/Renderer.h>
#include <Core/Utilities/Matrix4x4.h>

using namespace Nova;

TilemapSystem2D::TilemapSystem2D() 
{
	m_ecs = Services::Fetch<ECS>();
}

void TilemapSystem2D::Update(float dt) 
{
	for (EntityID e : entities)
	{
		auto* t  = m_ecs->GetComponent<Transform2D>(e);
		auto* tm = m_ecs->GetComponent<Tilemap2D>(e);

		// Skip updating children if transform unchanged
		auto& tCached = m_cachedTransforms[e];
		if (t->position == tCached.position 
			&& t->rotation == tCached.rotation 
			&& t->scale == tCached.scale) continue;

		// Update child entity transforms
		for (auto& [childEntity, type] : tm->GetChildEntities()) 
		{
			auto* tChild = m_ecs->GetComponent<Transform2D>(childEntity);
			tChild->position = t->position;
			tChild->rotation = t->rotation;
			tChild->scale	 = t->scale;
		}

		tCached = *t;
	}
}

void TilemapSystem2D::OnEntityAdded(EntityID e)
{
	auto* t  = m_ecs->GetComponent<Transform2D>(e);
	auto* tm = m_ecs->GetComponent<Tilemap2D>(e);

	const std::vector<Tile2D>& tileset = tm->GetTileset();
	const std::vector<TilemapLayer2D>& layers = tm->GetLayers();

	int mapW = tm->GetMapWidth();
	int mapH = tm->GetMapHeight();

	int tileW = tm->GetTileWidth();
	int tileH = tm->GetTileHeight();

	// Height and width of map in pixels
	int pixelW = mapW * tileW;
	int pixelH = mapH * tileH;

	// Calculate the offset needed to start drawing at the top-left of a symmetric Orthographic projection
	float orthoOffsetX = -(pixelW / 2.0f);
	float orthoOffsetY = -(pixelH / 2.0f);

	Renderer* renderer = Services::Fetch<Renderer>();

	for (size_t layerIdx = 0; layerIdx < layers.size(); layerIdx++)
	{
		const TilemapLayer2D& layer = layers[layerIdx];

		Texture* layerTexture = new Texture();
		if (!layerTexture->CreateRenderTexture(pixelW, pixelH))
		{
			Debug::LogError("Failed to create render texture for layer %zu", layerIdx);
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
				if (tileIndex < 0 || tileIndex >= (int)tileset.size()) continue;

				const Tile2D& tile = tileset[tileIndex];
				if (!tile.tileset) continue;

				// Apply the projection offset so (0,0) maps to the top-left of the FBO
				float x = orthoOffsetX + ((float)col * tileW);
				float y = orthoOffsetY + ((float)row * tileH);
				float w = (float)tileW;
				float h = (float)tileH;

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

		EntityID layerEntity = m_ecs->CreateEntity();
		m_ecs->AddComponent(layerEntity, Transform2D(t->position, t->rotation, t->scale));
		m_ecs->AddComponent(layerEntity, Sprite2D(layerTexture, 1, 1, (int)layerIdx));

		tm->AddChildEntity(layerEntity, TilemapChildType2D::LayerSprite);
	}

	const std::vector<TilemapRegion2D>& collisionRegions = tm->GetCollisionRegions();

	for (const TilemapRegion2D& region : collisionRegions)
	{
		float w = (float)(region.endCol - region.startCol) * tileW;
		float h = (float)(region.endRow - region.startRow) * tileH;

		float x = (float)region.startCol * tileW * t->scale.x;
		float y = (float)region.startRow * tileH * t->scale.y;

		EntityID colliderEntity = m_ecs->CreateEntity();
		m_ecs->AddComponent(colliderEntity, Transform2D(Vector2(x, y), 0.0f, t->scale));
		m_ecs->AddComponent(colliderEntity, RigidBody2D(Vector2(w / 2.0f, h / 2.0f), w, h, true, false));

		tm->AddChildEntity(colliderEntity, TilemapChildType2D::RectCollider);
	}

	// Cache transform component to check for changes
	m_cachedTransforms[e] = *t;
}

void TilemapSystem2D::OnEntityRemoved(EntityID e)
{
	m_cachedTransforms.erase(e);
}

void TilemapSystem2D::OnAllEntitiesRemoved()
{
	m_cachedTransforms.clear();
}