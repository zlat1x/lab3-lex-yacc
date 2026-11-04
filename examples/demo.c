
int main()
{
    int sum = 0;
    int i = 1;

    while (i <= 5) {
        sum = sum + i;
        i = i + 1;
    }

    printf("%d\n", sum);

    if (sum == 15) {
        printf("%d\n", 100);
    } else {
        printf("%d\n", 0);
    }

    return 0;
}
