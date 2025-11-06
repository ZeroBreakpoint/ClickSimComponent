#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ClickSimComponent.generated.h"


class IInputProcessor;


UCLASS(ClassGroup = (Input), meta = (BlueprintSpawnableComponent))
class YOURPROJECTNAME_API UClickSimComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UClickSimComponent();

	// Optional manual trigger (also bound automatically to Gamepad A).
	UFUNCTION(BlueprintCallable, Category = "ClickSim")
	void SimulateLeftClick();

	//If true, only click when at least one visible/hovered UMG widget exists.
	UPROPERTY(EditAnywhere, Category = "ClickSim")
	bool bRequireHoveredWidget = false;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	// Binds input when the PlayerController's InputComponent exists.
	void TryBindInput();

	// Clears widget focus and returns focus to the game viewport.
	void EnsureViewportFocus() const;

	// Disable focus navigation across all live UMG widgets (Stops DPAD/A-button focus routing)
	void DisableUMGFocusNavigation() const;

	// Make every widget in the current widget trees non-focusable (runtime safety net)
	void ForceWidgetsNonFocusable() const;

	// ---- NEW: Slate preprocessor registration helpers ----
	void RegisterPreprocessor();
	void UnregisterPreprocessor();

	// Stored registration handle
	TSharedPtr<IInputProcessor> Preprocessor;  // keep alive while component is active

	FTimerHandle BindRetryTimer;
};