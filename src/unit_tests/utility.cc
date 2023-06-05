#include "testing.h"
#include <filesystem>
#include <random>
#include <vector>

TEST_CASE("utility") {
  SUBCASE("IteratorRange") {
    std::array<int, 5> arr0 = {1, 2, 3, 4, 5};
    for (auto &i : mi::IteratorRange(arr0)) i++;
    for (auto &i : mi::IteratorRange(arr0.data(), 5)) i++;
    for (auto &i : mi::IteratorRange(arr0.begin(), arr0.end())) i++;
    CHECK(arr0[0] == 1 + 3);
    CHECK(arr0[1] == 2 + 3);
    CHECK(arr0[2] == 3 + 3);
    CHECK(arr0[3] == 4 + 3);
    CHECK(arr0[4] == 5 + 3);
  }
  SUBCASE("ranges::adjacent") {
    std::array<int, 5> arr0 = {1, 2, 3, 4, 5};
    std::array<int, 5> arr1 = {6, 7, 8, 9, 10};
    for (auto &&[a, b] : mi::ranges::adjacent<2>(arr0, false)) a += b;
    for (auto &&[a, b] : mi::ranges::adjacent<2>(arr1, true) | std::views::drop(1)) a += b;
    CHECK(arr0[0] == 1 + 2);
    CHECK(arr0[1] == 2 + 3);
    CHECK(arr0[2] == 3 + 4);
    CHECK(arr0[3] == 4 + 5);
    CHECK(arr0[4] == 5);
    CHECK(arr1[0] == 6);
    CHECK(arr1[1] == 7 + 8);
    CHECK(arr1[2] == 8 + 9);
    CHECK(arr1[3] == 9 + 10);
    CHECK(arr1[4] == 10 + 6);
  }
  SUBCASE("ranges::zip") {
    std::array<int, 5> arr0 = {1, 2, 3, 4, 5};
    std::array<int, 5> arr1 = {6, 7, 8, 9, 10};
    for (auto &&[a, b] : mi::ranges::zip(arr0, arr1) | std::views::drop(1)) a += b;
    CHECK(arr0[0] == 1);
    CHECK(arr0[1] == 2 + 7);
    CHECK(arr0[2] == 3 + 8);
    CHECK(arr0[3] == 4 + 9);
    CHECK(arr0[4] == 5 + 10);
  }
  SUBCASE("Half") {
    SUBCASE("Test accuracy") {
      auto prng = PRNG();
      for (int check = 0; check < 1024; check++) {
        float u0 = std::generate_canonical<float, 32>(prng);
        float x0 = (2 * u0 - 1) * 65504.0f;
        float xh = mi::Half(x0);
        CHECK(mi::fabs(xh - x0) / mi::fabs(x0) <= 0x1p-11f);
      }
    }
    SUBCASE("Test subnormal conversion") {
      CHECK(mi::Half(0x1p-14f).isnormal() == true);
      CHECK(mi::Half(0x1p-14f * 1023.0f / 1024.0f).isnormal() == false);
      CHECK(float(mi::Half(+0x1p-14f / 1024.0f)) == +0x1p-14f / 1024.0f);
      CHECK(float(mi::Half(-0x1p-14f / 1024.0f)) == -0x1p-14f / 1024.0f);
      CHECK(float(mi::Half(+0x1p-14f / 1024.0f).decrement()) == +0.0f);
      CHECK(float(mi::Half(-0x1p-14f / 1024.0f).increment()) == -0.0f);
      CHECK(float(mi::Half(std::numeric_limits<float>::denorm_min())) == 0.0f);
    }
    SUBCASE("Test Inf conversion") {
      mi::Half posInf(+mi::constants::Inf<float>);
      mi::Half negInf(-mi::constants::Inf<float>);
      CHECK(posInf.isinf());
      CHECK(negInf.isinf());
      CHECK(posInf.signbit() == false);
      CHECK(negInf.signbit() == true);
      CHECK(mi::isinf(float(posInf)));
      CHECK(mi::isinf(float(negInf)));
      CHECK(mi::signbit(float(posInf)) == false);
      CHECK(mi::signbit(float(negInf)) == true);

      // Test infinity by overflow.
      posInf = mi::Half(+70000.0f);
      negInf = mi::Half(-70000.0f);
      CHECK(posInf.isinf());
      CHECK(negInf.isinf());
      CHECK(posInf.signbit() == false);
      CHECK(negInf.signbit() == true);
      CHECK(mi::isinf(float(posInf)));
      CHECK(mi::isinf(float(negInf)));
      CHECK(mi::signbit(float(posInf)) == false);
      CHECK(mi::signbit(float(negInf)) == true);
    }
    SUBCASE("Test NaN conversion") {
      mi::Half posNaN(+mi::constants::NaN<float>);
      mi::Half negNaN(-mi::constants::NaN<float>);
      CHECK(posNaN.isnan());
      CHECK(negNaN.isnan());
      CHECK(posNaN.signbit() == false);
      CHECK(negNaN.signbit() == true);
      CHECK(mi::isnan(float(posNaN)));
      CHECK(mi::isnan(float(negNaN)));
      CHECK(mi::signbit(float(posNaN)) == false);
      CHECK(mi::signbit(float(negNaN)) == true);
    }
  }
  SUBCASE("Algorithm") {
    SUBCASE("Min and max") {
      CHECK(mi::sqr(1.5) == 1.5 * 1.5);
      CHECK(mi::min(2, 3) == 2);
      CHECK(mi::max(2, 3) == 3);
      int valueA = 1;
      int valueB = 2;
      CHECK(&mi::minReference(valueA, valueB) == &valueA);
      CHECK(&mi::maxReference(valueA, valueB) == &valueB);
      CHECK(mi::clamp(0.5, 0.6, 0.8) == 0.6);
      CHECK(mi::clamp(0.7, 0.6, 0.8) == 0.7);
      CHECK(mi::clamp(0.9, 0.6, 0.8) == 0.8);
    }
    SUBCASE("Integer operations") {
      CHECK(mi::roundUpTo<8>(5) == 8);
      CHECK(mi::roundUpTo<11>(49) == 55);
      CHECK(mi::factorial(4) == 4 * 3 * 2 * 1);
      CHECK(mi::factorial(5) == 5 * 4 * 3 * 2 * 1);
      CHECK(mi::factorial(6) == 6 * 5 * 4 * 3 * 2 * 1);
      CHECK(mi::choose(5, 0) == 1);
      CHECK(mi::choose(5, 1) == 5);
      CHECK(mi::choose(5, 2) == 10);
      CHECK(mi::choose(5, 3) == 10);
      CHECK(mi::choose(5, 4) == 5);
      CHECK(mi::choose(5, 5) == 1);
      CHECK(mi::nthPow(2.0, +4) == 16.0);
      CHECK(mi::nthPow(2.0, +5) == 32.0);
      CHECK(mi::nthPow(4.0, -2) == 1.0 / 16.0);
    }
    SUBCASE("Base64 encode/decode") {
      auto prng = PRNG();
      std::vector<uint8_t> bytes;
      for (int i = 0; i < 1024; i++) bytes.push_back(uint8_t(prng(256)));
      CHECK(bytes != mi::encodeBase64(bytes));
      CHECK(bytes == mi::decodeBase64(mi::encodeBase64(bytes)));
    }
  }
  SUBCASE("String") {
    SUBCASE("Character classes") {
      CHECK(mi::char_class::alnum('H'));
      CHECK(mi::char_class::alnum('7'));
      CHECK(mi::char_class::alnum('@') == false);
      CHECK(mi::char_class::punct('@'));
      CHECK(mi::char_class::word('H'));
      CHECK(mi::char_class::word('7'));
      CHECK(mi::char_class::word('_'));
      CHECK(mi::char_class::word('-') == false);
      CHECK(mi::char_class::word('$') == false);
      CHECK(!(!mi::char_class::word)('H'));
    }
    SUBCASE("Lower and upper case") {
      CHECK(mi::toLower('B') == 'b');
      CHECK(mi::toUpper('h') == 'H');
      CHECK(mi::toLower("Hello, world!") == "hello, world!");
      CHECK(mi::toUpper(L"Hello, world!") == L"HELLO, WORLD!");
      CHECK(mi::icaseEqual("foo", "foo"));
      CHECK(mi::icaseEqual("foo", "FoO"));
      CHECK(mi::icaseLess("heLlO, wORLd!", "GoodBYE, WorLd!") == mi::icaseLess("Hello, world!", "Goodbye, world!"));
      CHECK(mi::icaseGreater("heLlO, wORLd!", "GoodBYE, WorLd!") == mi::icaseGreater("Hello, world!", "Goodbye, world!"));
    }
    SUBCASE("Space trimming") {
      const char *withSpace = "  \t\nHello, world!\n\t \t";
      CHECK(mi::trim(withSpace) == "Hello, world!");
      CHECK(mi::trimLeft(withSpace) == "Hello, world!\n\t \t");
      CHECK(mi::trimRight(withSpace) == "  \t\nHello, world!");
      CHECK(mi::show("Hello, world!") == "\"Hello, world!\"");
      CHECK(mi::show("\"Hello\n, world!\t\t\"") == "\"\\\"Hello\\n, world!\\t\\t\\\"\"");
    }
    SUBCASE("String conversion") {
      CHECK(mi::toString(1.25) == "1.25");
      CHECK(mi::toString(123) == "123");
      CHECK(mi::toString(true) == "true");
      CHECK(mi::toString(false) == "false");
      CHECK(mi::toString(mi::constants::NaN<float>) == "nan");
      CHECK(mi::toString(mi::constants::Inf<float>) == "inf");
      CHECK(mi::toString(-mi::constants::Inf<float>) == "-inf");
      CHECK(mi::stringTo<int>("0xfbc37") == 0xfbc37); // Hex
      CHECK(mi::stringTo<int>("0b10010") == 0b10010); // Binary
      CHECK(mi::stringTo<int>("0173422") == 0173422); // Octal
      CHECK(mi::stringTo<bool>("True") == true);
      CHECK(mi::stringTo<bool>("faLSe") == false);
      CHECK(mi::stringTo<bool>("1") == true);
      CHECK(mi::stringTo<bool>("0") == false);
      CHECK(mi::stringTo<float>("+inf") == +mi::constants::Inf<float>);
      CHECK(mi::stringTo<float>("-inf") == -mi::constants::Inf<float>);
      CHECK(mi::isnan(mi::stringTo<float>("nan")));
      CHECK_THROWS(mi::stringTo<bool>("maybe"));
      CHECK_THROWS(mi::stringTo<unsigned>("-73"));
      CHECK_THROWS(mi::stringTo<unsigned long>("bar"));
    }
    SUBCASE("Load and save") {
      std::string text = "Hello, world!";
      auto temp = std::filesystem::temp_directory_path();
      CHECK_NOTHROW(mi::saveStringToFile((temp / "Test.txt").string(), text));
      CHECK(text == mi::loadFileToString((temp / "Test.txt").string()));
    }
    SUBCASE("Split") {
      SUBCASE("With skipEmpty=true") {
        auto tokens = mi::SplitString("foo, bar, baz", mi::char_class::these(", "), /*skipEmpty=*/true);
        CHECK(tokens.size() == 3);
        CHECK(tokens.at(0) == "foo");
        CHECK(tokens.at(1) == "bar");
        CHECK(tokens.at(2) == "baz");
        auto [foo, bar, baz] = tokens.destructure<3>();
        CHECK(foo == "foo");
        CHECK(bar == "bar");
        CHECK(baz == "baz");
      }
      SUBCASE("With skipEmpty=false") {
        auto tokens = mi::SplitString("foo/bar//baz", mi::char_class::these("/"), /*skipEmpty=*/false);
        CHECK(tokens.size() == 4);
        CHECK(tokens.at(0) == "foo");
        CHECK(tokens.at(1) == "bar");
        CHECK(tokens.at(2) == "");
        CHECK(tokens.at(3) == "baz");
      }
    }
  }
}
