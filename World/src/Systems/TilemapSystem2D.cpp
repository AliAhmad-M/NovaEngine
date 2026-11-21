#include <World/Systems/TilemapSystem2D.h>
#include <Core/Services.h>
#include <Core/Debug.h>
#include <Graphics/Components/Sprite2D.h>
#include <Physics/Components/RigidBody2D.h>
#include <World/Components/Tilemap2D.h>
#include <Graphics/Renderer.h>
#include <Core/Utilities/Matrix4x4.h>

#include <cmath>

using namespace Nova;

// World position of a collision region's origin, given the tilemap's world transform
static Vector2 ColliderOrigin(const TilemapRegion2D& region,
	int tileW, int tileH,
	float orthoOffsetX, float orthoOffsetY,
	const WorldTransform2D& world)
{
	float localX = (orthoOffsetX + (float)region.startCol * tileW) * world.scale.x;
	float localY = (orthoOffsetY + (float)region.startRow * tileH) * world.scale.y;

	float rad = world.rotation * (3.14159265358979f / 180.0f);
	float c = std::cos(rad);
	float s = std::sin(rad);

	return Vector2(
		world.position.x + (localX * c - localY * s),
		world.position.y + (localX * s + localY * c)
	);
}

TilemapSystem2D::TilemapSystem2D()
{
	m_ecsRef = Services::Fetch<ECS>();
}

void TilemapSystem2D::Update(float dt)
{
	for (EntityID e : entities)
	{
		auto* t = m_ecsRef->GetComponent<WorldTransform2D>(e);
		auto* tm = m_ecsRef->GetComponent<Tilemap2D>(e);
		if (!t || !tm) continue;

		// Skip updating children if transform unchanged
		auto& tCached = m_cachedTransforms[e];
		if (t->position == tCached.position
			&& t->rotation == tCached.rotation
			&& t->scale == tCached.scale) continue;

		const int tileW = tm->GetTileWidth();
		const int tileH = tm->GetTileHeight();
		const float orthoOffsetX = -((tm->GetMapWidth() * tileW) / 2.0f);
		const float orthoOffsetY = -((tm->GetMapHeight() * tileH) / 2.0f);
		const std::vector<TilemapRegion2D>& regions = tm->GetCollisionRegions();

		// Update child entity transforms
		size_t colliderIdx = 0;
		for (auto& [childEntity, type] : tm->GetChildEntities())
		{
			auto* tChild = m_ecsRef->GetComponent<Transform2D>(childEntity);

			if (type == TilemapChildType2D::LayerSprite)
			{
				if (!tChild) continue;
				tChild->position = t->position;
				tChild->rotation = t->rotation;
				tChild->scale = t->scale;
			}
			else
			{
				size_t idx = colliderIdx++;
				if (!tChild || idx >= regions.size()) continue;

				// Each collider keeps its own offset inside the map
				tChild->position = ColliderOrigin(regions[idx], tileW, tileH, orthoOffsetX, orthoOffsetY, *t);
				tChild->rotation = t->rotation;
				tChild->scale = t->scale;
			}
		}

		tCached = *t;
	}
}

void TilemapSystem2D::OnEntityAdded(EntityID e)
{
	auto* t = m_ecsRef->GetComponent<Transform2D>(e);
	auto* world = m_ecsRef->GetComponent<WorldTransform2D>(e);
	auto* tm = m_ecsRef->GetComponent<Tilemap2D>(e);

	if (!t || !tm) return;
	if (!tm->GetChildEntities().empty()) return;

	const WorldTransform2D initialWorld = world
		? *world
		: WorldTransform2D{ t->position, t->rotation, t->scale };

	const std::vector<Tile2D>& tileset = tm->GetTileset();
	const std::vector<TilemapLayer2D>& layers = tm->GetLayers();

	int mapW = tm->GetMapWidth();
	int mapH = tm->GetMapHeight();

	int tileW = tm->GetTileWidth();
	int tileH = tm->GetTileHeight();

	// Height and width of map in pixels
	int pixelW = mapW * tileW;
	int pixelH = mapH * tileH;

	// Calculate the offset needed to start drawing
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
					tile.tx, tile.ty, tile.tw, tile.th,
					Color::WHITE, 0.0f
				);
			}
		}

		renderer->Flush();
		layerTexture->UnbindFBO();

		EntityID layerEntity = m_ecsRef->CreateEntity();
		m_ecsRef->AddComponent(layerEntity, Transform2D(initialWorld.position, initialWorld.rotation, initialWorld.scale));
		m_ecsRef->AddComponent(layerEntity, WorldTransform2D{ initialWorld.position, initialWorld.rotation, initialWorld.scale });
		m_ecsRef->AddComponent(layerEntity, Sprite2D(layerTexture, 1, 1, (int)layerIdx));

		tm->AddChildEntity(layerEntity, TilemapChildType2D::LayerSprite);
	}

	// Colliders start from the tilemap's own transform, until the world transform is computed
	const WorldTransform2D colliderWorld{ t->position, t->rotation, t->scale };

	const std::vector<TilemapRegion2D>& collisionRegions = tm->GetCollisionRegions();

	for (const TilemapRegion2D& region : collisionRegions)
	{
		float w = (float)(region.endCol - region.startCol) * tileW;
		float h = (float)(region.endRow - region.startRow) * tileH;

		Vector2 origin = ColliderOrigin(region, tileW, tileH, 0.0f, 0.0f, colliderWorld);

		EntityID colliderEntity = m_ecsRef->CreateEntity();
		m_ecsRef->AddComponent(colliderEntity, Transform2D(origin, colliderWorld.rotation, colliderWorld.scale));
		m_ecsRef->AddComponent(colliderEntity, WorldTransform2D{ origin, colliderWorld.rotation, colliderWorld.scale });
		m_ecsRef->AddComponent(colliderEntity, RigidBody2D(Vector2(w / 2.0f, h / 2.0f), w, h, RigidBodyType2D::Static, false));

		tm->AddChildEntity(colliderEntity, TilemapChildType2D::RectCollider);
	}

	// Cache transform component to check for changes
	m_cachedTransforms[e] = initialWorld;
}

void TilemapSystem2D::OnEntityRemoved(EntityID e)
{
	m_cachedTransforms.erase(e);
}

void TilemapSystem2D::OnAllEntitiesRemoved()
{
	m_cachedTransforms.clear();
}