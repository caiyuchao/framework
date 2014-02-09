
#include <stdio.h>
#include <logging.h>

int main(void)
{
    log_init(NULL);
    LOG_ERROR("test\n");
    return 0;
}
