#include <World/Systems/TilemapSystem2D.h>
#include <Core/Services.h>
#include <Core/Components/Transform2D.h>
#include <Graphics/Components/Sprite2D.h>
#include <Physics/Components/RigidBody2D.h>
#include <Graphics/Renderer.h>
#include <Core/Utilities/Matrix4x4.h>

void TilemapSystem2D::OnEntityAdded(EntityID e)
{
	ECS* ecs = Services::Fetch<ECS>();

	Transform2D* transform = ecs->GetComponent<Transform2D>(e);
	Tilemap2D* tilemap = ecs->GetComponent<Tilemap2D>(e);

	const std::vector<Tile2D>& tileset = tilemap->GetTileset();
	const std::vector<TilemapLayer2D>& layers = tilemap->GetLayers();

	int mapW = tilemap->GetMapWidth();
	int mapH = tilemap->GetMapHeight();

	int tileW = tilemap->GetTileWidth();
	int tileH = tilemap->GetTileHeight();

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
			EngineDebug::LogError("Failed to create render texture for layer %zu", layerIdx);
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

		EntityID layerEntity = ecs->CreateEntity();
		ecs->AddComponent(layerEntity, Transform2D(transform->position, transform->rotation, transform->scale));
		ecs->AddComponent(layerEntity, Sprite2D(layerTexture, 1, 1, (int)layerIdx));

		tilemap->AddChildEntity(layerEntity, TilemapChildType2D::LayerSprite);
	}

	const std::vector<TilemapRegion2D>& collisionRegions = tilemap->GetCollisionRegions();

	for (const TilemapRegion2D& region : collisionRegions)
	{
		float w = (float)(region.endCol - region.startCol) * tileW;
		float h = (float)(region.endRow - region.startRow) * tileH;

		float x = (float)region.startCol * tileW * transform->scale.x;
		float y = (float)region.startRow * tileH * transform->scale.y;

		EntityID colliderEntity = ecs->CreateEntity();
		ecs->AddComponent(colliderEntity, Transform2D(Vector2(x, y), 0.0f, transform->scale));
		ecs->AddComponent(colliderEntity, RigidBody2D(Vector2(w / 2.0f, h / 2.0f), w, h, true, false));

		tilemap->AddChildEntity(colliderEntity, TilemapChildType2D::RectCollider);
	}
}

void TilemapSystem2D::OnEntityRemoved(EntityID e)
{
	ECS* ecs = Services::Fetch<ECS>();

	Tilemap2D* tilemap = ecs->GetComponent<Tilemap2D>(e);
	if (!tilemap) return;

	for (const auto& [childEntity, type] : tilemap->GetChildEntities())
	{
		ecs->DeleteEntity(childEntity);
	}

	tilemap->ClearChildEntities();
}