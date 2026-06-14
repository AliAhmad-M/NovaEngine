#include <Core/ECS/SystemManager.h>

void SystemManager::OnEntityDestroyed(EntityID entity) {
	for (auto const& pair : m_systems) {
		// Remove deleted entity from all systems
		auto const& system = pair.second;
		if (system->entities.count(entity)) {
			system->entities.erase(entity);
			system->OnEntityRemoved(entity);
		}
	}
}

void SystemManager::OnAllEntitesDestroyed() {
	for (auto const& pair : m_systems) {
		auto const& system = pair.second;

		system->entities.clear();
		system->OnAllEntitiesRemoved();
	}
}

void SystemManager::OnEntitySignatureChanged(EntityID entity, Signature entitySignature) {
	for (auto const& pair : m_systems) {
		auto const& type = pair.first;
		auto const& system = pair.second;

		// Get system signature
		auto const& systemSignature = m_signatures[type];
		
		bool matches = (entitySignature & systemSignature) == systemSignature;
		bool isTracked = system->entities.count(entity) > 0;

		// If entity signature matched and entity is not already added
		if (matches && !isTracked) {
			// Add the entity to system
			system->entities.insert(entity);
			system->OnEntityAdded(entity);
		}

		// If entity signature not matched and entity is already added
		else if (!matches && isTracked) {
			// Remove entity from system
			system->entities.erase(entity);
			system->OnEntityRemoved(entity);
		}
	}
}
