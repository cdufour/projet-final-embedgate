#pragma once
// Version de la passerelle, écrite dans la ligne START du journal
// (INTERFACES.md §5). À faire évoluer à chaque tag : 0.2.0 en P2, etc.
namespace embedgate {
const char* version();
}
