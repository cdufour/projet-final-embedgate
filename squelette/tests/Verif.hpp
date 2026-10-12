#pragma once
// ============================================================================
// Verif.hpp — FOURNI : un mini-outil de tests, sans bibliothèque externe
//
// PRINCIPE. Chaque fichier tests/test_xxx.cpp est un PETIT PROGRAMME : son
// main() lance des fonctions de test, puis renvoie 0 si tout est vert, 1
// sinon. CMake en fait l'exécutable build/test_xxx et le déclare à ctest.
//
//   #include "Verif.hpp"
//
//   void remontage()
//   {
//       ...
//       VERIFIER(fs.existe("a"));                              // condition vraie
//       VERIFIER_EGAL(fs.liste().size(), 2u);                  // deux valeurs égales
//       VERIFIER_LEVE(fs.supprimer("x"), FichierIntrouvable);  // exception levée
//       EXIGER(!fs.liste().empty());     // condition vraie, sinon ARRÊTER ce test
//   }
//
//   int main()
//   {
//       verif::lancer("remontage", remontage);
//       return verif::resultat();
//   }
//
// C'est assert() du C, en mieux :
//   - un échec affiche le fichier, la ligne et, pour VERIFIER_EGAL, les deux
//     valeurs ; les vérifications suivantes continuent (sauf avec EXIGER) ;
//   - une exception imprévue dans un test le fait échouer, sans empêcher les
//     tests suivants. Pour vérifier qu'une instruction NE lève PAS
//     d'exception, il suffit donc de l'écrire.
//
// LANCER :
//   ctest --test-dir build --output-on-failure   tous les fichiers de tests
//   ./build/test_coupures                         un seul, avec tout son affichage
// ============================================================================

// Ce fichier est traité comme une bibliothèque du système : ses éventuels
// avertissements (comparaison signé / non signé dans VERIFIER_EGAL…) ne sont
// pas affichés, comme avec les outils de test professionnels.
#pragma GCC system_header

#include <atomic>     // std::atomic : compteur partagé sans risque entre threads
#include <exception>  // std::exception
#include <iostream>   // std::cout, std::cerr
#include <sstream>    // std::ostringstream : fabriquer un texte avec <<
#include <string>
#include <type_traits>
#include <utility>    // std::declval

namespace verif {

// Nombre de vérifications en échec. `std::atomic` car plusieurs threads
// peuvent échouer en même temps (test de concurrence).
inline std::atomic<int> nbEchecs{0};

// Levée par EXIGER pour arrêter le test en cours (attrapée par lancer()).
struct Arret {};

// Signaler un échec : compter, et afficher où il a eu lieu.
inline void echec(const char* fichier, int ligne, const std::string& message)
{
    ++nbEchecs;
    std::cerr << fichier << ':' << ligne << ": ÉCHEC : " << message << '\n';
}

// --- Afficher une valeur dans un message d'échec, quand c'est possible.
// (Détail technique : Affichable<T> vaut vrai si « std::cout << valeur » compile.)
template <typename T, typename = void>
struct Affichable : std::false_type {};
template <typename T>
struct Affichable<T, std::void_t<decltype(std::declval<std::ostream&>() << std::declval<const T&>())>>
    : std::true_type {};

template <typename T>
std::string enTexte(const T& valeur)
{
    std::ostringstream os;
    if constexpr (std::is_same_v<T, unsigned char> || std::is_same_v<T, signed char>)
        os << static_cast<int>(valeur);  // un octet s'affiche comme un nombre
    else if constexpr (Affichable<T>::value)
        os << valeur;
    else
        os << "(valeur non affichable)";
    return os.str();
}

// Utilisée par VERIFIER_EGAL.
template <typename A, typename B>
void egal(const A& a, const B& b, const char* texteA, const char* texteB, const char* fichier, int ligne)
{
    if (!(a == b))
        echec(fichier, ligne,
              std::string("VERIFIER_EGAL(") + texteA + ", " + texteB + ") : " + enTexte(a) + " != " + enTexte(b));
}

// Lancer une fonction de test et afficher son verdict.
template <typename Fonction>
void lancer(const char* nom, Fonction test)
{
    const int avant = nbEchecs;
    try {
        test();
    } catch (const Arret&) {
        // EXIGER a échoué : l'échec est déjà compté et affiché.
    } catch (const std::exception& e) {
        ++nbEchecs;
        std::cerr << nom << " : ÉCHEC : exception imprévue : " << e.what() << '\n';
    } catch (...) {
        ++nbEchecs;
        std::cerr << nom << " : ÉCHEC : exception imprévue\n";
    }
    std::cout << (nbEchecs == avant ? "[ OK    ] " : "[ ÉCHEC ] ") << nom << std::endl;
}

// Le code de retour de main() : 0 si tout est vert, 1 sinon.
inline int resultat()
{
    if (nbEchecs == 0) {
        std::cout << "Tous les tests sont verts.\n";
        return 0;
    }
    std::cout << nbEchecs << " vérification(s) en échec.\n";
    return 1;
}

} // namespace verif

// --- Les macros. Ce sont des macros (et non des fonctions) pour connaître
// le fichier et la ligne (__FILE__, __LINE__) et le texte de la vérification
// (#condition). `do { … } while (0)` permet de les utiliser comme une
// instruction ordinaire, suivie d'un point-virgule.

#define VERIFIER(condition)                                                          \
    do {                                                                             \
        if (!(condition))                                                            \
            ::verif::echec(__FILE__, __LINE__, "VERIFIER(" #condition ")");          \
    } while (0)

#define VERIFIER_EGAL(a, b) ::verif::egal((a), (b), #a, #b, __FILE__, __LINE__)

#define VERIFIER_LEVE(instruction, TypeException)                                    \
    do {                                                                             \
        const char* probleme_ = "aucune exception levée";                            \
        try {                                                                        \
            instruction;                                                             \
        } catch (const TypeException&) {                                             \
            probleme_ = nullptr;                                                     \
        } catch (...) {                                                              \
            probleme_ = "une AUTRE exception a été levée";                           \
        }                                                                            \
        if (probleme_)                                                               \
            ::verif::echec(__FILE__, __LINE__,                                       \
                           std::string("VERIFIER_LEVE(" #instruction ", " #TypeException \
                                       ") : ") + probleme_);                         \
    } while (0)

// Comme VERIFIER, mais arrête le test en cours en cas d'échec (à éviter dans
// un thread : utilisez VERIFIER).
#define EXIGER(condition)                                                            \
    do {                                                                             \
        if (!(condition)) {                                                          \
            ::verif::echec(__FILE__, __LINE__, "EXIGER(" #condition ")");            \
            throw ::verif::Arret{};                                                  \
        }                                                                            \
    } while (0)
