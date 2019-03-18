#include <iostream>
#include <iomanip>
#include <sstream>
#include <string>
#include <cassert>

using namespace std;

const int BLOCK_SIZE = 512 / sizeof(uint32_t) / 8; // число целых чисел на блок
const int BLOCK_BYTES = 64; // число байт на блок

// циклический сдвиг влево на bits бит
uint32_t Rol(uint32_t value, size_t bits) {
	return (value << bits) | (value >> (32 - bits));
}

// выполнение преобразований над блоком block
void Transform(uint32_t *digest, uint32_t block[BLOCK_SIZE]) {
	uint32_t a = digest[0];
	uint32_t b = digest[1];
	uint32_t c = digest[2];
	uint32_t d = digest[3];
	uint32_t e = digest[4];

	for (int i = 0; i < 80; i++) {
		if (i >= 16)
			block[i & 15] = Rol(block[(i + 13) & 15] ^ block[(i + 8) & 15] ^ block[(i + 2) & 15] ^ block[i & 15], 1);

        if (0 <= i && i < 20) {
            e += ((b & (c ^ d)) ^ d) + 0x5A827999;
        }
        else if (20 <= i && i < 40) {
            e += (b ^ c ^ d) + 0x6ED9EBA1;
        }
        else if (40 <= i && i < 60) {
            e += (((b | c) & d) | (b & c)) + 0x8F1BBCDC;
        }
        else if (60 <= i && i < 80) {
            e += (b ^ c ^ d) + 0xCA62C1D6;
        }

        uint32_t tmp = e + block[i & 15] + Rol(a, 5);
        e = d;
        d = c;
        c = Rol(b, 30);
        b = a;
        a = tmp;
    }

	digest[0] += a;
	digest[1] += b;
	digest[2] += c;
	digest[3] += d;
	digest[4] += e;
}

// разбиение буфера на блоки
void SplitToBlock(const string& buffer, uint32_t block[BLOCK_SIZE]) {
	for (int i = 0; i < BLOCK_SIZE; i++) {
		block[i] = 0;

		block[i] += (buffer[4 * i + 0] & 255) << 24;
		block[i] += (buffer[4 * i + 1] & 255) << 16;
		block[i] += (buffer[4 * i + 2] & 255) << 8;
		block[i] += (buffer[4 * i + 3] & 255) << 0;
	}
}

// паддинг, преобразование и сохранение хеша в строку
string Final(uint32_t *digest, string& buffer, uint64_t &transforms) {
	uint64_t total_bits = (transforms * BLOCK_BYTES + buffer.size()) * 8;

	buffer += (char) 0x80; // добавляем 1

	size_t orig_size = buffer.size();

	// добавляем нули
	while (buffer.size() < BLOCK_BYTES)
		buffer += (char) 0;

	uint32_t block[BLOCK_SIZE];
	SplitToBlock(buffer, block);

	if (orig_size > BLOCK_BYTES - 8) {
		Transform(digest, block);
		transforms++;

		for (size_t i = 0; i < BLOCK_SIZE - 2; i++)
			block[i] = 0;
	}

	block[BLOCK_SIZE - 1] = (uint32_t) total_bits;
	block[BLOCK_SIZE - 2] = (uint32_t) (total_bits >> 32);

	Transform(digest, block);

	ostringstream result;

	// сохраняем результат хеширования
	for (size_t i = 0; i < 5; i++)
		result << hex << setfill('0') << setw(8) << digest[i];

	return result.str();
}

// хеширование
string SHA1(const string& s) {
	uint32_t digest[5] = { 0x67452301, 0xefcdab89, 0x98badcfe, 0x10325476, 0xc3d2e1f0 };
	string buffer = "";
	uint64_t transforms = 0;

	istringstream is(s);

	while (1) {
		char sbuf[BLOCK_BYTES];

		is.read(sbuf, BLOCK_BYTES);
		buffer.append(sbuf, (size_t)is.gcount());

		if (buffer.size() != BLOCK_BYTES)
			break;

		uint32_t block[BLOCK_SIZE];
		SplitToBlock(buffer, block);

		Transform(digest, block);
		transforms++;

		buffer.clear();
	}

	return Final(digest, buffer, transforms);
}

// проверка корректности работы алгоритма на нескольких "известных" хешах
void test() {
	cout << "Testing SHA1... ";

	assert(SHA1("") == "da39a3ee5e6b4b0d3255bfef95601890afd80709");
	assert(SHA1("abc") == "a9993e364706816aba3e25717850c26c9cd0d89d");
	assert(SHA1("The quick brown fox jumps over the lazy dog") == "2fd4e1c67a2d28fced849ee1bb76e7391b93eb12");
	assert(SHA1("The quick brown fox jumps over the lazy cog") == "de9f2c7fd25e1b3afad3e85a0bd17d9b100db4b3");

	cout << "OK" << endl;
}

int main() {
	test();

	string s;

	cout << "Enter string: ";
	getline(cin, s); // считываем строку

	string hash = SHA1(s); // получаем хеш

	cout << hash; // выводим хеш
}