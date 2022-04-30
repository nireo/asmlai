int main() {
  int x = 10;
  for (int i = 0; i < 5; ++i) {
    x += i;
  }

  if (x < 10) {
    return -1;
  } else {
    return x;
  }
}
