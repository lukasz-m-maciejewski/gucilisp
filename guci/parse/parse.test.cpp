#include "catch2/catch.hpp"
#include "guci/parse/parse.hpp"

namespace guci {

TEST_CASE("is_whitespace", "[parse]") {
  REQUIRE(is_whitespace(' ') == true);
  REQUIRE(is_whitespace('a') == false);
}

TEST_CASE("skip_one_of", "[parse]") {
  MaybeParse mp = skip_one_of("abc")("aab");

  PartialParse p = mp.value();

  REQUIRE(p.t == NIL);
  REQUIRE(p.rest == "ab");
}

TEST_CASE("parse_identifier", "[parse]") {
  SECTION("alpha begin") {
    PartialParse p = parse_identifier(" aa? 123").value();

    REQUIRE(p.t == Identifier("aa?"));
    REQUIRE(p.rest == " 123");
  }
  SECTION("plus begin") {
    PartialParse p = parse_identifier(" + 123").value();

    REQUIRE(p.t == Identifier("+"));
    REQUIRE(p.rest == " 123");
  }
}

TEST_CASE("parse_number", "[parse]") {
  SECTION("positive number") {
    PartialParse p = parse_number(" 123").value();

    REQUIRE(p.t == Number(123));
    REQUIRE(p.rest == "");
  }
  SECTION("negative number") {
    PartialParse p = parse_number(" -4321   ").value();

    REQUIRE(p.t == Number(-4321));
    REQUIRE(p.rest == "   ");
  }
}

TEST_CASE("parse_atom", "[parse]") {
  SECTION("parse number") {
    PartialParse p = parse_atom("42 dups").value();

    REQUIRE(p.t == Number(42));
    REQUIRE(p.rest == " dups");
  }
  SECTION("parse identifier") {
    PartialParse p = parse_atom("dups 42").value();

    REQUIRE(p.t == Identifier("dups"));
    REQUIRE(p.rest == " 42");
  }
}

TEST_CASE("parse_list", "[parse]") {
  SECTION("list of numbers") {
    PartialParse p = parse_list("(1 2 3)").value();

    auto expected_term = List<Term>{Number(1), Number(2), Number(3)};
    REQUIRE(p.rest == "");
    REQUIRE(p.t == expected_term);
  }
  SECTION("list with identifier at first pos") {
    PartialParse p = parse_list("(+ 1 2 3)").value();

    auto expected_term =
        List<Term>{Identifier("+"), Number(1), Number(2), Number(3)};
    REQUIRE(p.rest == "");
    REQUIRE(p.t == expected_term);
  }
  SECTION("list with identifier at first pos and a nested list") {
    PartialParse p = parse_list("(+ (+ 3 4) 2)").value();

    auto expected_term = List<Term>{
        Identifier("+"),
        List<Term>{Identifier("+"), Number(3), Number(4)},
        Number(2),
    };
    REQUIRE(p.rest == "");
    REQUIRE(p.t == expected_term);
  }
}

TEST_CASE("parse_string", "[parse]") {
  SECTION("parse correct string without escape sequences") {
    PartialParse p = parse_string(" \"abc\"  x").value();

    auto expected_term = String("abc");
    REQUIRE(p.t == expected_term);
    REQUIRE(p.rest == "  x");
  }
  SECTION("parse correct string with escape sequences") {
    PartialParse p = parse_string(" \"abc\\ndef\"  x").value();

    auto expected_term = String("abc\ndef");
    REQUIRE(p.t == expected_term);
    REQUIRE(p.rest == "  x");
  }
}

}  // namespace guci
