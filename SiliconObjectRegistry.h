
#ifndef SILICON_OBJECT_REGISTRY_H
#define SILICON_OBJECT_REGISTRY_H

class SiliconObjectDefinition;

// Initialize and cleanup registry
extern void InitializeSiliconObjects();
extern void CleanupSiliconObjects();

// Lookup silicon object by name
extern SiliconObjectDefinition *LookupObjectDefinition(const char *object);


#endif

