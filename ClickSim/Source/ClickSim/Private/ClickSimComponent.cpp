#include "ClickSimComponent.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "Framework/Application/SlateApplication.h"
#include "Input/Events.h"
#include "InputCoreTypes.h"
#include "TimerManager.h"
#include "Widgets/SWindow.h"
#include "GenericPlatform/GenericWindow.h"
#include "Blueprint/UserWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Framework/Application/IInputProcessor.h"

class FClickSimInputPreprocessor : public IInputProcessor
{
public:
	explicit FClickSimInputPreprocessor(TWeakObjectPtr<UClickSimComponent> InOwner)
		: Owner(InOwner) {
	}

	virtual ~FClickSimInputPreprocessor() {}

	virtual const TCHAR* GetDebugName() const override { return TEXT("ClickSimPreprocessor"); }

	virtual bool HandleKeyDownEvent(FSlateApplication& SlateApp, const FKeyEvent& InKeyEvent) override
	{
		if (InKeyEvent.GetKey() == EKeys::Gamepad_FaceButton_Bottom)
		{
			if (UClickSimComponent* C = Owner.Get())
			{
				// Trigger our cursor-based click and CONSUME the key so UMG can't handle it.
				C->SimulateLeftClick();
				return true; // handled
			}
		}
		return false; // not handled, continue normal routing
	}

	virtual void Tick(const float DeltaTime, FSlateApplication& SlateApp, TSharedRef<ICursor> Cursor) override
	{
		// No per-frame work required; we only care about KeyDown events.
	}

private:
	TWeakObjectPtr<UClickSimComponent> Owner;
};
// ---------------------------------------------------------

UClickSimComponent::UClickSimComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	bAutoActivate = true;
}

void UClickSimComponent::BeginPlay()
{
	Super::BeginPlay();


}

void UClickSimComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	UnregisterPreprocessor();
	Super::EndPlay(EndPlayReason);
}

void UClickSimComponent::TryBindInput()
{
	APlayerController* PC = Cast<APlayerController>(GetOwner());
	if (!PC) return;

	if (!PC->InputComponent)
	{
		GetWorld()->GetTimerManager().SetTimer(BindRetryTimer, this, &UClickSimComponent::TryBindInput, 0.05f, false);
		return;
	}

	PC->bShowMouseCursor = true;
	PC->bEnableClickEvents = true;
	PC->bEnableMouseOverEvents = true;

	FInputModeGameAndUI Mode;
	Mode.SetHideCursorDuringCapture(false);
	Mode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	PC->SetInputMode(Mode);

	PC->InputComponent->BindKey(EKeys::Gamepad_FaceButton_Bottom, IE_Pressed,
		this, &UClickSimComponent::SimulateLeftClick);

}

void UClickSimComponent::RegisterPreprocessor()
{
	if (Preprocessor.IsValid())
		return;

	TSharedRef<FClickSimInputPreprocessor> PP = MakeShared<FClickSimInputPreprocessor>(TWeakObjectPtr<UClickSimComponent>(this));
	FSlateApplication::Get().RegisterInputPreProcessor(PP);
	Preprocessor = PP;
}

void UClickSimComponent::UnregisterPreprocessor()
{
	if (Preprocessor.IsValid())
	{
		FSlateApplication::Get().UnregisterInputPreProcessor(Preprocessor.ToSharedRef());
		Preprocessor.Reset();
	}
}

void UClickSimComponent::EnsureViewportFocus() const
{
	FSlateApplication::Get().ClearAllUserFocus();
	UWidgetBlueprintLibrary::SetFocusToGameViewport();
}

void UClickSimComponent::SimulateLeftClick()
{
	FSlateApplication& Slate = FSlateApplication::Get();

	// Break any stale capture just in case
	Slate.ReleaseMouseCapture();


	// Require a hovered widget of any type
	if (bRequireHoveredWidget)
	{
		TArray<UUserWidget*> All;
		UWidgetBlueprintLibrary::GetAllWidgetsOfClass(GetWorld(), All, UUserWidget::StaticClass(), false);
		bool bHasHover = false;
		for (UUserWidget* W : All)
		{
			if (W && W->IsVisible() && W->GetIsEnabled() && W->IsHovered())
			{
				bHasHover = true; break;
			}
		}
		if (!bHasHover) return;
	}

	// Resolve native window if available
	TSharedPtr<SWindow> TopWin = Slate.GetActiveTopLevelWindow();
	if ((!TopWin.IsValid() || !TopWin->GetNativeWindow().IsValid()) && GEngine && GEngine->GameViewport)
	{
		if (TSharedPtr<SWindow> ViewWin = GEngine->GameViewport->GetWindow())
		{
			TopWin = ViewWin;
		}
	}
	const TSharedPtr<FGenericWindow> NativeWin = TopWin.IsValid() ? TopWin->GetNativeWindow()
		: TSharedPtr<FGenericWindow>();

	// Build pointer events at the real cursor position
	const FVector2D ScreenPos = Slate.GetCursorPos();
	const int32     PointerIdx = 0;
	const FModifierKeysState Mods;

	// Hover refresh “nudge”
	{
		const FVector2D LastPos = ScreenPos + FVector2D(0.5f, 0.0f);
		const FPointerEvent NudgeMove(PointerIdx, ScreenPos, LastPos, TSet<FKey>(), EKeys::Invalid, 0, Mods);
		Slate.ProcessMouseMoveEvent(NudgeMove);

		const FPointerEvent MoveEvt(PointerIdx, ScreenPos, ScreenPos, TSet<FKey>(), EKeys::Invalid, 0, Mods);
		Slate.ProcessMouseMoveEvent(MoveEvt);
	}

	const FPointerEvent DownEvt(PointerIdx, ScreenPos, ScreenPos,
		TSet<FKey>({ EKeys::LeftMouseButton }), EKeys::LeftMouseButton, 0, Mods);
	const FPointerEvent UpEvt(PointerIdx, ScreenPos, ScreenPos,
		TSet<FKey>(), EKeys::LeftMouseButton, 0, Mods);

	// Dispatch mouse down/up
	auto CallMouseDown = [&](auto& App, const TSharedPtr<FGenericWindow>& Win, const FPointerEvent& E) -> void
		{
#if (__cplusplus >= 202002L) || (_MSVC_LANG >= 202002L)
			if constexpr (requires { App.ProcessMouseButtonDownEvent(Win, E); })
				App.ProcessMouseButtonDownEvent(Win, E);
			else
				App.ProcessMouseButtonDownEvent(E);
#else
			App.ProcessMouseButtonDownEvent(Win.IsValid() ? Win : TSharedPtr<FGenericWindow>(), E);
#endif
		};
	auto CallMouseUp = [&](auto& App, const TSharedPtr<FGenericWindow>& Win, const FPointerEvent& E) -> void
		{
#if (__cplusplus >= 202002L) || (_MSVC_LANG >= 202002L)
			if constexpr (requires { App.ProcessMouseButtonUpEvent(Win, E); })
				App.ProcessMouseButtonUpEvent(Win, E);
			else
				App.ProcessMouseButtonUpEvent(E);
#else
			App.ProcessMouseButtonUpEvent(Win.IsValid() ? Win : TSharedPtr<FGenericWindow>(), E);
#endif
		};

	CallMouseDown(Slate, NativeWin, DownEvt);
	CallMouseUp(Slate, NativeWin, UpEvt);

	if (bReturnFocusToViewportAfterClick)
	{
		PollMenusAndRefocus();
	}
}

void UClickSimComponent::SetEnabled(bool bNewEnabled)
{
	TryBindInput();
	RegisterPreprocessor();

	// Keep cursor/UI behavior consistent at start.
	EnsureViewportFocus();

	// Optional: these were previously unconditional
	if (bDisableUMGFocusNavOnBeginPlay)
	{
		DisableUMGFocusNavigation();
	}
	if (bForceWidgetsNonFocusableOnBeginPlay)
	{
		ForceWidgetsNonFocusable();
	}

	if (UWorld* W = GetWorld())
	{
		FTimerHandle Tmp;
		W->GetTimerManager().SetTimer(Tmp, this, &UClickSimComponent::EnsureViewportFocus, 0.15f, false);
	}


	if (bNewEnabled)
	{
		RegisterPreprocessor();
	}
	else
	{
		UnregisterPreprocessor();
	}
}

void UClickSimComponent::SetDisabled(bool bNewDisabled)
{

	if (bNewDisabled)
	{
		UnregisterPreprocessor();
	}
	else
	{
		RegisterPreprocessor();
	}
}

void UClickSimComponent::DisableUMGFocusNavigation() const
{
	// Walk every live UUserWidget and set all navigation rules to Stop
	TArray<UUserWidget*> All;
	UWidgetBlueprintLibrary::GetAllWidgetsOfClass(GetWorld(), All, UUserWidget::StaticClass(), false);

	for (UUserWidget* Root : All)
	{
		if (!Root || !Root->WidgetTree) continue;

		TArray<UWidget*> Widgets;
		Root->WidgetTree->GetAllWidgets(Widgets);

		for (UWidget* W : Widgets)
		{
			if (!W) continue;

			// Ensure a UWidgetNavigation object exists
			if (!W->Navigation)
			{
				W->Navigation = NewObject<UWidgetNavigation>(W, NAME_None, RF_Transient);
			}

			if (W->Navigation)
			{
				W->SetNavigationRule(EUINavigation::Up, EUINavigationRule::Stop, NAME_None);
				W->SetNavigationRule(EUINavigation::Down, EUINavigationRule::Stop, NAME_None);
				W->SetNavigationRule(EUINavigation::Left, EUINavigationRule::Stop, NAME_None);
				W->SetNavigationRule(EUINavigation::Right, EUINavigationRule::Stop, NAME_None);
				W->SetNavigationRule(EUINavigation::Next, EUINavigationRule::Stop, NAME_None);
				W->SetNavigationRule(EUINavigation::Previous, EUINavigationRule::Stop, NAME_None);
			}
		}
	}
}

void UClickSimComponent::ForceWidgetsNonFocusable() const
{
	// Make every widget non-focusable so A-press doesn’t trigger the previously-focused one
	TArray<UUserWidget*> All;
	UWidgetBlueprintLibrary::GetAllWidgetsOfClass(GetWorld(), All, UUserWidget::StaticClass(), false);

	for (UUserWidget* Root : All)
	{
		if (!Root || !Root->WidgetTree) continue;

		TArray<UWidget*> Widgets;
		Root->WidgetTree->GetAllWidgets(Widgets);

		for (UWidget* W : Widgets)
		{
			if (!W) continue;

			// Only UUserWidget (and some specific controls) expose SetIsFocusable at runtime.
			if (UUserWidget* UW = Cast<UUserWidget>(W))
			{
				UW->SetIsFocusable(false);
			}
			// (Optional) Add specific controls (e.g., editable text) as needed.
		}
	}
}