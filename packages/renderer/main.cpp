import nova.platform.window;

int main() {
    nova::platform::window window{
        {
            .title = "Nova",
            .size = {.width = 1600, .height = 900},
            .resizable = true
        }
    };

    while(!window.should_close()) {
        window.poll_events();
    }

    return 0;
}
