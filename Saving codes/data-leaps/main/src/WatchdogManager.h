class WatchdogManager {
    public:
        static void init(int timeout_ms, bool panic = true);

        static void addToCurrentTask();

        static void removeFromCurrentTask();

        // Deve ser chamado periodicamente para nao dar timeout
        static void reset();
};