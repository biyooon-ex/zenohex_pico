idf_component_register(
    # ソース
    SRCS "zxp_term.c" "config.c" "query.c" "sample.c" "session.c" "session_option.c" "time_compat.c" "timestamp.c" "zenohex_pico.c"
    # 依存関係
    PRIV_REQUIRES "libatomvm" "zenoh_pico_component"
    # オブジェクトファイルを強制的にリンクするオプション
    WHOLE_ARCHIVE
    # 実装した NIF は外部 API を持たず AtomVM から呼び出されないため、
    # WHOLE_ARCHIVE を指定をしないとリンカがオブジェクトファイルを不要と判断してリンクされません。
)
