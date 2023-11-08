#include "component.hpp"

Bind::Bind()
{

}

Bind::~Bind()
{

}

void Bind::Dereference()
{
    count -= 1;
}

void Bind::Reference()
{
    count += 1;
}


Component::Component()
{

}

Component::~Component()
{
    
}

ComponentManager::ComponentManager()
{

}

ComponentManager::~ComponentManager()
{

}