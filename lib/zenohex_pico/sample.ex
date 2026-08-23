defmodule ZenohexPico.Sample do
  @moduledoc false

  @type t :: %__MODULE__{
          attachment: binary() | nil,
          congestion_control: :block | :drop,
          encoding: String.t(),
          express: boolean(),
          key_expr: String.t(),
          kind: :put | :delete,
          payload: binary(),
          priority:
            :real_time
            | :interactive_high
            | :interactive_low
            | :data_high
            | :data
            | :data_low
            | :background,
          timestamp: String.t() | nil
        }

  defstruct attachment: nil,
            congestion_control: :block,
            encoding: "zenoh/bytes",
            express: false,
            key_expr: "",
            kind: :put,
            payload: <<>>,
            priority: :data,
            timestamp: nil
end
