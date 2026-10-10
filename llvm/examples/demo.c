/* Compile with Clang's ANC216 target. Expected return value: 42. */
__attribute__((noinline)) int add(int a, int b)
{
    return a + b;
}

int main(void)
{
    volatile int answer = add(40, 2);
    return answer;
}
