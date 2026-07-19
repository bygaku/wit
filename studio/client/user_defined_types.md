# QTreeWidgetItemについて

---

## カスタムのアイテムを定義する
> QTreeWidgetItem::UserType(1000)以降がユーザ定義の空間となっている。

- QTreeWidgetItem::UserType + 1: CueCollection
- QTreeWidgetItem::UserType + 2: Cue
- QTreeWidgetItem::UserType + 3: Waveform


## 謎1: UniquePtrやSharedPtrが使えない
> Resolve: QObjectかQWidgetか親子関係にあると、親が削除されたときにQtがスマートポインタの役割を果たすため、ダブルフリーを起こすらしい。