# WORK_LOG

## Phase 1 Step 1-4 stability v16: real visual-order onion and less aggressive eraser

### 変更内容
- `src/ui/AppDrawingMode.cpp`
  - オニオンスキンを通常キャンバス描画の後に重ねて描くよう変更。
  - 前フレーム青 / 次フレーム赤のDirect DrawList表示を維持。
  - 消しゴムの当たり判定から `stroke.radiusPx` 加算を外し、`eraserStroke.radiusPx` そのものを判定半径に使用。
  - 消しゴム分割用のサンプリング間隔を細かくし、触れた部分だけ抜けやすくした。

### 目的
- オニオンスキンがTexture描画で隠れて見えない環境を避ける。
- 消しゴムがストローク全体を消したように見える過剰判定を避ける。

### ビルド
```powershell
cmake --build .\build
```

### 実行確認
- `③ 作画` を開く。
- `Step 1-4 stability pass v16` 表示を確認。
- 1フレーム目に線、2フレーム目を選択して前オニオンONで青い線が見える。
- 消しゴムで線の中央だけをなぞり、線全体ではなく触れた部分だけ抜ける。
