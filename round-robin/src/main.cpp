#include <iostream>
#include <queue>

static std::queue<int> thread_ids;
static int c_timeslice = 0;
static int ttick_invocations = 0;
static int on_cpu = -1;

static void context_switch() {
    ttick_invocations = 0;
    if ( thread_ids.size() > 0 ) {
        on_cpu = thread_ids.front();
        thread_ids.pop();
    } else on_cpu = -1;
}

/**
 * Функция будет вызвана перед каждым тестом, если вы
 * используете глобальные и/или статические переменные
 * не полагайтесь на то, что они заполнены 0 - в них
 * могут храниться значения оставшиеся от предыдущих
 * тестов.
 *
 * timeslice - квант времени, который нужно использовать.
 * Поток смещается с CPU, если пока он занимал CPU функция
 * timer_tick была вызвана timeslice раз.
 **/
void scheduler_setup(int timeslice)
{
    /* Put your code here */
    // clear queue
    std::queue<int> empty;
    thread_ids.swap( empty );
    c_timeslice = timeslice;
    ttick_invocations = 0;
    on_cpu = -1;
}

/**
 * Функция вызывается, когда создается новый поток управления.
 * thread_id - идентификатор этого потока и гарантируется, что
 * никакие два потока не могут иметь одинаковый идентификатор.
 **/
void new_thread(int thread_id)
{
    /* Put your code here */
    if ( on_cpu == -1 )
        on_cpu = thread_id;
    else thread_ids.push( thread_id );
}

/**
 * Функция вызывается, когда поток, исполняющийся на CPU,
 * завершается. Завершится может только поток, который сейчас
 * исполняется, поэтому thread_id не передается. CPU должен
 * быть отдан другому потоку, если есть активный
 * (незаблокированный и незавершившийся) поток.
 **/
void exit_thread()
{
    /* Put your code here */
    context_switch();
}

/**
 * Функция вызывается, когда поток, исполняющийся на CPU,
 * блокируется. Заблокироваться может только поток, который
 * сейчас исполняется, поэтому thread_id не передается. CPU
 * должен быть отдан другому активному потоку, если таковой
 * имеется.
 **/
void block_thread()
{
    /* Put your code here */
    context_switch();
}

/**
 * Функция вызывается, когда один из заблокированных потоков
 * разблокируется. Гарантируется, что thread_id - идентификатор
 * ранее заблокированного потока.
 **/
void wake_thread(int thread_id)
{
    /* Put your code here */
    if ( on_cpu == -1 )
        on_cpu = thread_id;
    else thread_ids.push(thread_id);
}

/**
 * Ваш таймер. Вызывается каждый раз, когда проходит единица
 * времени.
 **/
void timer_tick()
{
    /* Put your code here */
    ttick_invocations++;
    if ( on_cpu == -1 ) return;
    
    if ( ttick_invocations == c_timeslice ) {
        thread_ids.push( on_cpu );
        context_switch();
    }
}

/**
 * Функция должна возвращать идентификатор потока, который в
 * данный момент занимает CPU, или -1 если такого потока нет.
 * Единственная ситуация, когда функция может вернуть -1, это
 * когда нет ни одного активного потока (все созданные потоки
 * либо уже завершены, либо заблокированы).
 **/
int current_thread()
{
    /* Put your code here */
    return on_cpu;
}

int main( int argc, char** argv ) {
    scheduler_setup(1);
    new_thread(0);
    new_thread(1);
    new_thread(2);

    std::cout << current_thread() << std::endl;
    timer_tick();

    std::cout << current_thread() << std::endl;
    timer_tick();

    std::cout << current_thread() << std::endl;
    timer_tick();
    return 0;
}