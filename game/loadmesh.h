#pragma once

#include "mesh.h"

#include <memory>
#include <string>

std::unique_ptr<Mesh> loadMesh(const std::string &path);
