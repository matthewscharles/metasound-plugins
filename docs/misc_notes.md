# Misc notes

## Arrays
- [TArray](https://dev.epicgames.com/documentation/en-us/unreal-engine/array-containers-in-unreal-engine)
  - Add()
  - SetNum() 
  - Num() 
  
## Updating from initial tutorial

### C4996

```bash
warning C4996: 'Metasound::FCreateOperatorParams': Update APIs to use FBuildOperatorParams. Operator creation routines should have the following signature "TUniquePtr<CreateOperator(const FBuildOperatorParams&, FBuildResults&)"
```
New signature:
```C++
static TUniquePtr<IOperator> CreateOperator(const FBuildOperatorParams& InParams, FBuildResults& OutErrors)
```

